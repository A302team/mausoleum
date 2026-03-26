#ifdef _WIN32
#include "network/platform/NetworkEventLoopWin.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include "common/logging/Logger.h"
#include "network/platform/SocketPlatform.h"

namespace {
constexpr int kIoWaitTimeoutMs = 100;
}

void WindowsNetworkEventLoop::udpLoop(std::atomic<bool>& running,
                                      std::vector<NetworkUdpEndpoint>& udpEndpoints,
                                      ConcurrentQueue<NetPacket>& inboundQueue) {
    while (running) {
        if (udpEndpoints.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        std::vector<WSAPOLLFD> pollfds;
        pollfds.reserve(udpEndpoints.size());
        for (const auto& ep : udpEndpoints) {
            WSAPOLLFD pfd{};
            pfd.fd = ep.sock;
            pfd.events = POLLRDNORM;
            pollfds.push_back(pfd);
        }

        int ready = WSAPoll(pollfds.data(), static_cast<ULONG>(pollfds.size()), kIoWaitTimeoutMs);
        if (ready == SOCK_ERROR) {
            int err = GetSocketPlatform().lastError();
            LOG_ERROR("NetworkIO", "UDP WSAPoll 오류: " << err);
            continue;
        }
        if (ready == 0) continue;

        for (size_t i = 0; i < pollfds.size(); ++i) {
            if ((pollfds[i].revents & POLLRDNORM) == 0) continue;
            const auto& ep = udpEndpoints[i];

            while (running) {
                std::vector<char> buffer;
                buffer.resize(ep.maxPacketSize);

                sockaddr_in sender{};
                SockLenType addrLen = sizeof(sender);
                int bytesRecv = recvfrom(ep.sock, buffer.data(), static_cast<int>(buffer.size()),
                                         0, reinterpret_cast<sockaddr*>(&sender), &addrLen);
                if (bytesRecv == SOCK_ERROR) {
                    int err = GetSocketPlatform().lastError();
                    if (err == ERR_CONNRESET || err == ERR_WOULDBLOCK) break;
                    LOG_ERROR("NetworkIO", "UDP 수신 실패: " << err);
                    break;
                }

                buffer.resize(bytesRecv);
                NetPacket packet;
                packet.protocol = NetProtocol::Udp;
                packet.addr = sender;
                packet.data = std::move(buffer);
                inboundQueue.push(std::move(packet));
            }
        }
    }
}

void WindowsNetworkEventLoop::tcpLoop(std::atomic<bool>& running,
                                      std::vector<NetworkTcpListener>& tcpListeners,
                                      ConnectionRegistry& connections,
                                      ConcurrentQueue<NetPacket>& inboundQueue) {
    while (running) {
        auto clients = connections.snapshot();
        if (tcpListeners.empty() && clients.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        struct PollEntry {
            bool isListener = false;
            ConnectionId connectionId = 0;
            SocketType sock = INVALID_SOCK;
            sockaddr_in addr{};
        };

        std::vector<WSAPOLLFD> pollfds;
        std::vector<PollEntry> entries;
        pollfds.reserve(tcpListeners.size() + clients.size());
        entries.reserve(tcpListeners.size() + clients.size());

        for (const auto& listener : tcpListeners) {
            WSAPOLLFD pfd{};
            pfd.fd = listener.sock;
            pfd.events = POLLRDNORM;
            pollfds.push_back(pfd);

            PollEntry entry{};
            entry.isListener = true;
            entry.sock = listener.sock;
            entries.push_back(entry);
        }

        for (const auto& client : clients) {
            WSAPOLLFD pfd{};
            pfd.fd = client.sock;
            pfd.events = POLLRDNORM;
            pollfds.push_back(pfd);

            PollEntry entry{};
            entry.isListener = false;
            entry.connectionId = client.id;
            entry.sock = client.sock;
            entry.addr = client.addr;
            entries.push_back(entry);
        }

        int ready = WSAPoll(pollfds.data(), static_cast<ULONG>(pollfds.size()), kIoWaitTimeoutMs);
        if (ready == SOCK_ERROR) {
            int err = GetSocketPlatform().lastError();
            LOG_ERROR("NetworkIO", "TCP WSAPoll 오류: " << err);
            continue;
        }
        if (ready == 0) continue;

        std::vector<ConnectionId> toRemove;
        for (size_t i = 0; i < pollfds.size(); ++i) {
            const SHORT revents = pollfds[i].revents;
            if (revents == 0) continue;

            const PollEntry& entry = entries[i];
            if (entry.isListener) {
                while (running) {
                    sockaddr_in clientAddr{};
                    SockLenType addrLen = sizeof(clientAddr);
                    SocketType clientSock = accept(entry.sock,
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
                continue;
            }

            bool disconnected = (revents & (POLLHUP | POLLERR | POLLNVAL)) != 0;
            if (!disconnected && (revents & POLLRDNORM) != 0) {
                while (running) {
                    std::vector<char> buffer;
                    buffer.resize(4096);
                    int bytesRecv = recv(entry.sock, buffer.data(), static_cast<int>(buffer.size()), 0);
                    if (bytesRecv > 0) {
                        buffer.resize(bytesRecv);
                        NetPacket packet;
                        packet.protocol = NetProtocol::Tcp;
                        packet.connectionId = entry.connectionId;
                        packet.addr = entry.addr;
                        packet.data = std::move(buffer);
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
                toRemove.push_back(entry.connectionId);
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
