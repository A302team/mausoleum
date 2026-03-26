#if defined(__linux__)
#include "network/platform/NetworkEventLoopLinux.h"
#include <algorithm>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <sys/epoll.h>
#include "common/logging/Logger.h"
#include "network/platform/SocketPlatform.h"

namespace {
constexpr int kIoWaitTimeoutMs = 100;
}

void LinuxNetworkEventLoop::udpLoop(std::atomic<bool>& running,
                                    std::vector<NetworkUdpEndpoint>& udpEndpoints,
                                    ConcurrentQueue<NetPacket>& inboundQueue) {
    int epollFd = epoll_create1(0);
    if (epollFd == -1) {
        LOG_ERROR("NetworkIO", "UDP epoll 생성 실패: " << errno);
        return;
    }

    std::unordered_set<SocketType> registeredFds;
    std::vector<epoll_event> events(64);

    auto registerUdp = [&](const NetworkUdpEndpoint& endpoint) {
        if (!registeredFds.insert(endpoint.sock).second) {
            return;
        }

        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = endpoint.sock;
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, endpoint.sock, &ev) == -1 && errno != EEXIST) {
            LOG_ERROR("NetworkIO", "UDP epoll_ctl(ADD) 실패 fd=" << endpoint.sock << " err=" << errno);
            registeredFds.erase(endpoint.sock);
        }
    };

    while (running) {
        if (udpEndpoints.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        for (const auto& ep : udpEndpoints) {
            registerUdp(ep);
        }
        if (events.size() < udpEndpoints.size() * 2) {
            events.resize((std::max<size_t>)(64, udpEndpoints.size() * 2));
        }

        int ready = epoll_wait(epollFd, events.data(), static_cast<int>(events.size()), kIoWaitTimeoutMs);
        if (ready == -1) {
            if (errno == EINTR) continue;
            LOG_ERROR("NetworkIO", "UDP epoll_wait 오류: " << errno);
            continue;
        }
        if (ready == 0) continue;

        for (int i = 0; i < ready; ++i) {
            SocketType sock = events[i].data.fd;
            auto it = std::find_if(udpEndpoints.begin(), udpEndpoints.end(),
                                   [sock](const NetworkUdpEndpoint& ep) { return ep.sock == sock; });
            if (it == udpEndpoints.end()) {
                continue;
            }

            while (running) {
                std::vector<char> buffer;
                buffer.resize(it->maxPacketSize);

                sockaddr_in sender{};
                SockLenType addrLen = sizeof(sender);
                int bytesRecv = recvfrom(it->sock, buffer.data(), static_cast<int>(buffer.size()),
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

    CLOSE_SOCKET(epollFd);
}

void LinuxNetworkEventLoop::tcpLoop(std::atomic<bool>& running,
                                    std::vector<NetworkTcpListener>& tcpListeners,
                                    ConnectionRegistry& connections,
                                    ConcurrentQueue<NetPacket>& inboundQueue) {
    int epollFd = epoll_create1(0);
    if (epollFd == -1) {
        LOG_ERROR("NetworkIO", "TCP epoll 생성 실패: " << errno);
        return;
    }

    std::unordered_set<SocketType> listenerFds;
    std::unordered_map<SocketType, ConnectionId> clientFdToId;
    std::unordered_map<SocketType, sockaddr_in> clientFdToAddr;
    std::vector<epoll_event> events(128);

    auto addToEpoll = [&](SocketType sock, uint32_t eventMask) -> bool {
        epoll_event ev{};
        ev.events = eventMask;
        ev.data.fd = sock;
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, sock, &ev) == -1) {
            if (errno == EEXIST) return true;
            LOG_ERROR("NetworkIO", "TCP epoll_ctl(ADD) 실패 fd=" << sock << " err=" << errno);
            return false;
        }
        return true;
    };

    while (running) {
        auto clients = connections.snapshot();
        if (tcpListeners.empty() && clients.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        for (const auto& listener : tcpListeners) {
            if (!listenerFds.insert(listener.sock).second) continue;
            addToEpoll(listener.sock, EPOLLIN);
        }

        std::unordered_set<SocketType> snapshotClientFds;
        snapshotClientFds.reserve(clients.size());
        for (const auto& client : clients) {
            snapshotClientFds.insert(client.sock);
            auto [it, inserted] = clientFdToId.emplace(client.sock, client.id);
            if (inserted) {
                addToEpoll(client.sock, EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR);
            } else {
                it->second = client.id;
            }
            clientFdToAddr[client.sock] = client.addr;
        }

        for (auto it = clientFdToId.begin(); it != clientFdToId.end();) {
            if (!snapshotClientFds.contains(it->first)) {
                epoll_ctl(epollFd, EPOLL_CTL_DEL, it->first, nullptr);
                clientFdToAddr.erase(it->first);
                it = clientFdToId.erase(it);
                continue;
            }
            ++it;
        }

        const size_t desiredEventCount = (std::max<size_t>)(128, (tcpListeners.size() + clients.size()) * 2);
        if (events.size() < desiredEventCount) {
            events.resize(desiredEventCount);
        }

        int ready = epoll_wait(epollFd, events.data(), static_cast<int>(events.size()), kIoWaitTimeoutMs);
        if (ready == -1) {
            if (errno == EINTR) continue;
            LOG_ERROR("NetworkIO", "TCP epoll_wait 오류: " << errno);
            continue;
        }
        if (ready == 0) continue;

        std::vector<ConnectionId> toRemove;
        for (int i = 0; i < ready; ++i) {
            SocketType fd = events[i].data.fd;
            const uint32_t eventMask = events[i].events;

            if (listenerFds.contains(fd)) {
                while (running) {
                    sockaddr_in clientAddr{};
                    SockLenType addrLen = sizeof(clientAddr);
                    SocketType clientSock = accept(fd, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
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
                    clientFdToId[clientSock] = id;
                    clientFdToAddr[clientSock] = clientAddr;
                    addToEpoll(clientSock, EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR);
                    LOG_INFO("NetworkIO", "TCP 연결 수락: id=" << id);
                }
                continue;
            }

            auto mapIt = clientFdToId.find(fd);
            if (mapIt == clientFdToId.end()) {
                continue;
            }

            ConnectionId id = mapIt->second;
            bool disconnected = (eventMask & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)) != 0;
            if (!disconnected && (eventMask & EPOLLIN) != 0) {
                while (running) {
                    std::vector<char> buffer;
                    buffer.resize(4096);
                    int bytesRecv = recv(fd, buffer.data(), static_cast<int>(buffer.size()), 0);
                    if (bytesRecv > 0) {
                        buffer.resize(bytesRecv);
                        NetPacket packet;
                        packet.protocol = NetProtocol::Tcp;
                        packet.connectionId = id;
                        auto addrIt = clientFdToAddr.find(fd);
                        if (addrIt != clientFdToAddr.end()) {
                            packet.addr = addrIt->second;
                        }
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
                toRemove.push_back(id);
            }
        }

        std::sort(toRemove.begin(), toRemove.end());
        toRemove.erase(std::unique(toRemove.begin(), toRemove.end()), toRemove.end());
        for (ConnectionId id : toRemove) {
            ConnectionRegistry::TcpClient c{};
            if (connections.get(id, c)) {
                epoll_ctl(epollFd, EPOLL_CTL_DEL, c.sock, nullptr);
                clientFdToId.erase(c.sock);
                clientFdToAddr.erase(c.sock);
                GetSocketPlatform().closeSocket(c.sock);
            }
            connections.remove(id);
            LOG_INFO("NetworkIO", "TCP 연결 종료: id=" << id);
        }
    }

    CLOSE_SOCKET(epollFd);
}
#endif
