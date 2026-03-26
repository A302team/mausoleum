#ifdef _WIN32
#include "network/platform/NetworkEventLoopWin.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <vector>
#include "common/logging/Logger.h"
#include "network/platform/SocketPlatform.h"

namespace {
constexpr int kIoWaitTimeoutMs = 100;
constexpr int kIdleSleepMs = 50;
constexpr size_t kTcpReadBufferSize = 4096;
}

void WindowsNetworkEventLoop::udpLoop(std::atomic<bool>& running,
                                      const std::vector<NetworkUdpEndpoint>& udpEndpoints,
                                      ConcurrentQueue<NetPacket>& inboundQueue) {
    struct UdpEntry {
        int maxPacketSize = 0;
        std::vector<char> buffer;
    };

    std::unordered_map<SocketType, UdpEntry> udpByFd;
    udpByFd.reserve(udpEndpoints.size());
    for (const auto& ep : udpEndpoints) {
        if (ep.sock == INVALID_SOCK) continue;
        if (!GetSocketPlatform().setNonBlocking(ep.sock)) {
            LOG_ERROR("NetworkIO", "UDP 소켓 non-blocking 보장 실패 fd=" << ep.sock
                      << " err=" << GetSocketPlatform().lastError());
            continue;
        }

        UdpEntry entry;
        entry.maxPacketSize = (std::max)(ep.maxPacketSize, 256);
        entry.buffer.resize(static_cast<size_t>(entry.maxPacketSize));
        udpByFd.emplace(ep.sock, std::move(entry));
    }

    while (running) {
        if (udpByFd.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(kIdleSleepMs));
            continue;
        }

        fd_set readFds;
        fd_set errorFds;
        FD_ZERO(&readFds);
        FD_ZERO(&errorFds);

        std::vector<SocketType> watchedFds;
        watchedFds.reserve((std::min<size_t>)(udpByFd.size(), static_cast<size_t>(FD_SETSIZE)));

        for (const auto& kv : udpByFd) {
            SocketType fd = kv.first;
            if (watchedFds.size() >= static_cast<size_t>(FD_SETSIZE)) {
                LOG_ERROR("NetworkIO", "UDP select 감시 대상 초과: " << udpByFd.size()
                          << " (FD_SETSIZE=" << FD_SETSIZE << ")");
                break;
            }
            FD_SET(fd, &readFds);
            FD_SET(fd, &errorFds);
            watchedFds.push_back(fd);
        }

        if (watchedFds.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(kIdleSleepMs));
            continue;
        }

        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = kIoWaitTimeoutMs * 1000;
        int ready = select(0, &readFds, nullptr, &errorFds, &tv);
        if (ready == SOCK_ERROR) {
            int err = GetSocketPlatform().lastError();
            if (err == WSAEINTR) continue;
            LOG_ERROR("NetworkIO", "UDP select 오류: " << err);
            continue;
        }
        if (ready == 0) continue;

        for (SocketType fd : watchedFds) {
            const bool readable = FD_ISSET(fd, &readFds) != 0;
            const bool hasError = FD_ISSET(fd, &errorFds) != 0;
            if (!readable && !hasError) {
                continue;
            }
            if (hasError && !readable) {
                LOG_WARN("NetworkIO", "UDP 오류 이벤트 fd=" << fd);
                continue;
            }

            auto entryIt = udpByFd.find(fd);
            if (entryIt == udpByFd.end()) {
                continue;
            }
            auto& entry = entryIt->second;

            while (running) {
                sockaddr_in sender{};
                SockLenType addrLen = sizeof(sender);
                int bytesRecv = recvfrom(fd,
                                         entry.buffer.data(),
                                         static_cast<int>(entry.buffer.size()),
                                         0,
                                         reinterpret_cast<sockaddr*>(&sender),
                                         &addrLen);
                if (bytesRecv == SOCK_ERROR) {
                    int err = GetSocketPlatform().lastError();
                    if (err == WSAEMSGSIZE) {
                        LOG_WARN("NetworkIO", "UDP 패킷 잘림 발생 fd=" << fd << " buf=" << entry.buffer.size());
                        break;
                    }
                    if (err == ERR_CONNRESET || err == ERR_WOULDBLOCK) break;
                    LOG_ERROR("NetworkIO", "UDP 수신 실패 fd=" << fd << " err=" << err);
                    break;
                }

                NetPacket packet;
                packet.protocol = NetProtocol::Udp;
                packet.addr = sender;
                packet.data.assign(entry.buffer.begin(), entry.buffer.begin() + bytesRecv);
                inboundQueue.push(std::move(packet));
            }
        }
    }
}

void WindowsNetworkEventLoop::tcpLoop(std::atomic<bool>& running,
                                      const std::vector<NetworkTcpListener>& tcpListeners,
                                      ConnectionRegistry& connections,
                                      ConcurrentQueue<NetPacket>& inboundQueue) {
    std::vector<SocketType> listeners;
    listeners.reserve(tcpListeners.size());
    for (const auto& listener : tcpListeners) {
        if (listener.sock == INVALID_SOCK) continue;
        if (!GetSocketPlatform().setNonBlocking(listener.sock)) {
            LOG_ERROR("NetworkIO", "TCP 리스너 non-blocking 보장 실패 fd=" << listener.sock
                      << " err=" << GetSocketPlatform().lastError());
            continue;
        }
        listeners.push_back(listener.sock);
    }

    std::array<char, kTcpReadBufferSize> readBuffer{};

    while (running) {
        auto clients = connections.snapshot();
        if (listeners.empty() && clients.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(kIdleSleepMs));
            continue;
        }

        struct ClientEntry {
            ConnectionId id = 0;
            SocketType sock = INVALID_SOCK;
            sockaddr_in addr{};
        };

        fd_set readFds;
        fd_set errorFds;
        FD_ZERO(&readFds);
        FD_ZERO(&errorFds);

        std::vector<SocketType> watchedListeners;
        watchedListeners.reserve(listeners.size());
        std::vector<ClientEntry> watchedClients;
        watchedClients.reserve(clients.size());
        std::vector<ConnectionId> toRemove;

        auto tryWatchSocket = [&](SocketType fd) -> bool {
            const size_t watchedCount = watchedListeners.size() + watchedClients.size();
            if (watchedCount >= static_cast<size_t>(FD_SETSIZE)) {
                LOG_ERROR("NetworkIO", "TCP select 감시 대상 초과: " << (watchedCount + 1)
                          << " (FD_SETSIZE=" << FD_SETSIZE << ")");
                return false;
            }
            FD_SET(fd, &readFds);
            FD_SET(fd, &errorFds);
            return true;
        };

        for (SocketType sock : listeners) {
            if (!tryWatchSocket(sock)) {
                break;
            }
            watchedListeners.push_back(sock);
        }

        for (const auto& client : clients) {
            if (client.sock == INVALID_SOCK) continue;
            if (!GetSocketPlatform().setNonBlocking(client.sock)) {
                LOG_ERROR("NetworkIO", "클라이언트 non-blocking 보장 실패: id=" << client.id
                          << " err=" << GetSocketPlatform().lastError());
                toRemove.push_back(client.id);
                continue;
            }
            if (!tryWatchSocket(client.sock)) {
                break;
            }

            ClientEntry entry;
            entry.id = client.id;
            entry.sock = client.sock;
            entry.addr = client.addr;
            watchedClients.push_back(entry);
        }

        if (watchedListeners.empty() && watchedClients.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(kIdleSleepMs));
            continue;
        }

        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = kIoWaitTimeoutMs * 1000;
        int ready = select(0, &readFds, nullptr, &errorFds, &tv);
        if (ready == SOCK_ERROR) {
            int err = GetSocketPlatform().lastError();
            if (err != WSAEINTR) {
                LOG_ERROR("NetworkIO", "TCP select 오류: " << err);
            }
            ready = 0;
        }

        if (ready > 0) {
            for (SocketType listenerSock : watchedListeners) {
                const bool readable = FD_ISSET(listenerSock, &readFds) != 0;
                const bool hasError = FD_ISSET(listenerSock, &errorFds) != 0;
                if (!readable && !hasError) {
                    continue;
                }
                if (hasError && !readable) {
                    LOG_WARN("NetworkIO", "TCP 리스너 오류 이벤트 fd=" << listenerSock);
                    continue;
                }

                while (running) {
                    sockaddr_in clientAddr{};
                    SockLenType addrLen = sizeof(clientAddr);
                    SocketType clientSock = accept(listenerSock,
                                                   reinterpret_cast<sockaddr*>(&clientAddr),
                                                   &addrLen);
                    if (clientSock == INVALID_SOCK) {
                        int err = GetSocketPlatform().lastError();
                        if (err == ERR_WOULDBLOCK) break;
                        LOG_ERROR("NetworkIO", "TCP accept 실패: " << err);
                        break;
                    }

                    if (!GetSocketPlatform().setNonBlocking(clientSock)) {
                        LOG_ERROR("NetworkIO", "Non-blocking 설정 실패: " << GetSocketPlatform().lastError());
                        GetSocketPlatform().closeSocket(clientSock);
                        continue;
                    }

                    ConnectionId id = connections.add(clientSock, clientAddr);
                    LOG_INFO("NetworkIO", "TCP 연결 수락: id=" << id);
                }
            }

            for (const auto& entry : watchedClients) {
                const bool readable = FD_ISSET(entry.sock, &readFds) != 0;
                const bool hasError = FD_ISSET(entry.sock, &errorFds) != 0;
                if (!readable && !hasError) {
                    continue;
                }

                bool disconnected = hasError && !readable;
                if (readable) {
                    while (running) {
                        int bytesRecv = recv(entry.sock, readBuffer.data(), static_cast<int>(readBuffer.size()), 0);
                        if (bytesRecv > 0) {
                            NetPacket packet;
                            packet.protocol = NetProtocol::Tcp;
                            packet.connectionId = entry.id;
                            packet.addr = entry.addr;
                            packet.data.assign(readBuffer.begin(), readBuffer.begin() + bytesRecv);
                            inboundQueue.push(std::move(packet));
                            continue;
                        }

                        if (bytesRecv == 0) {
                            disconnected = true;
                            break;
                        }

                        int err = GetSocketPlatform().lastError();
                        if (err == ERR_WOULDBLOCK) {
                            break;
                        }
                        disconnected = true;
                        break;
                    }
                }

                if (disconnected) {
                    toRemove.push_back(entry.id);
                }
            }
        }

        std::sort(toRemove.begin(), toRemove.end());
        toRemove.erase(std::unique(toRemove.begin(), toRemove.end()), toRemove.end());
        for (ConnectionId id : toRemove) {
            ConnectionRegistry::TcpClient c{};
            if (connections.get(id, c)) {
                GetSocketPlatform().closeSocket(c.sock);
            }
            connections.remove(id);
            LOG_INFO("NetworkIO", "TCP 연결 종료: id=" << id);
        }
    }
}
#endif
