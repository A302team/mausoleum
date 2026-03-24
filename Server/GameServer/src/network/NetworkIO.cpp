#include "network/NetworkIO.h"
#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#ifndef _WIN32
#include <sys/epoll.h>
#include <cerrno>
#endif
#include "network/platform/SocketPlatform.h"

namespace {
constexpr int kIoWaitTimeoutMs = 100;
}

NetworkIO::NetworkIO(ConnectionRegistry& connections,
                     ConcurrentQueue<NetPacket>& inbound,
                     ConcurrentQueue<NetPacket>& outbound)
    : connections_(connections),
      inboundQueue_(inbound),
      outboundQueue_(outbound) {}

NetworkIO::~NetworkIO() {
    stop();
}

bool NetworkIO::start() {
    if (running_.exchange(true)) {
        return true;
    }

    if (!ensurePlatform()) {
        running_ = false;
        return false;
    }

    udpThread_ = std::thread([this]() { udpLoop(); });
    tcpThread_ = std::thread([this]() { tcpLoop(); });
    sendThread_ = std::thread([this]() { sendLoop(); });
    return true;
}

void NetworkIO::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    inboundQueue_.close();
    outboundQueue_.close();

    if (udpThread_.joinable()) udpThread_.join();
    if (tcpThread_.joinable()) tcpThread_.join();
    if (sendThread_.joinable()) sendThread_.join();

    closeAllSockets();

    if (platformStarted_) {
        GetSocketPlatform().shutdown();
        platformStarted_ = false;
    }
}

bool NetworkIO::addUdpEndpoint(int port, int maxPacketSize) {
    if (!ensurePlatform()) {
        return false;
    }
    SocketType sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCK) {
        LOG_ERROR("NetworkIO", "UDP 소켓 생성 실패: " << GetSocketPlatform().lastError());
        return false;
    }
    GetSocketPlatform().configureUdpSocket(sock);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCK_ERROR) {
        LOG_ERROR("NetworkIO", "UDP 바인딩 실패: " << GetSocketPlatform().lastError());
        CLOSE_SOCKET(sock);
        return false;
    }

    if (!setNonBlocking(sock)) {
        CLOSE_SOCKET(sock);
        return false;
    }

    UdpEndpoint endpoint;
    endpoint.sock = sock;
    endpoint.port = port;
    endpoint.maxPacketSize = (std::max)(maxPacketSize, 256);
    udpEndpoints_.push_back(endpoint);
    LOG_INFO("NetworkIO", "UDP 엔드포인트 등록: " << port);
    return true;
}

bool NetworkIO::addTcpListener(int port) {
    if (!ensurePlatform()) {
        return false;
    }
    SocketType sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCK) {
        LOG_ERROR("NetworkIO", "TCP 소켓 생성 실패: " << GetSocketPlatform().lastError());
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&opt), sizeof(opt));

    if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCK_ERROR) {
        LOG_ERROR("NetworkIO", "TCP 바인딩 실패: " << GetSocketPlatform().lastError());
        CLOSE_SOCKET(sock);
        return false;
    }

    if (listen(sock, SOMAXCONN) == SOCK_ERROR) {
        LOG_ERROR("NetworkIO", "TCP listen 실패: " << GetSocketPlatform().lastError());
        CLOSE_SOCKET(sock);
        return false;
    }

    if (!setNonBlocking(sock)) {
        CLOSE_SOCKET(sock);
        return false;
    }

    TcpListener listener;
    listener.sock = sock;
    listener.port = port;
    tcpListeners_.push_back(listener);
    LOG_INFO("NetworkIO", "TCP 리스너 등록: " << port);
    return true;
}

void NetworkIO::udpLoop() {
#ifdef _WIN32
    while (running_) {
        if (udpEndpoints_.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        std::vector<WSAPOLLFD> pollfds;
        pollfds.reserve(udpEndpoints_.size());
        for (const auto& ep : udpEndpoints_) {
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
            const auto& ep = udpEndpoints_[i];

            while (running_) {
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
                inboundQueue_.push(std::move(packet));
            }
        }
    }
#else
    int epollFd = epoll_create1(0);
    if (epollFd == -1) {
        LOG_ERROR("NetworkIO", "UDP epoll 생성 실패: " << errno);
        return;
    }

    std::unordered_set<SocketType> registeredFds;
    std::vector<epoll_event> events(64);

    auto registerUdp = [&](const UdpEndpoint& endpoint) {
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

    while (running_) {
        if (udpEndpoints_.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        for (const auto& ep : udpEndpoints_) {
            registerUdp(ep);
        }
        if (events.size() < udpEndpoints_.size() * 2) {
            events.resize((std::max<size_t>)(64, udpEndpoints_.size() * 2));
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
            auto it = std::find_if(udpEndpoints_.begin(), udpEndpoints_.end(),
                                   [sock](const UdpEndpoint& ep) { return ep.sock == sock; });
            if (it == udpEndpoints_.end()) {
                continue;
            }

            while (running_) {
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
                inboundQueue_.push(std::move(packet));
            }
        }
    }

    CLOSE_SOCKET(epollFd);
#endif
}

void NetworkIO::tcpLoop() {
#ifdef _WIN32
    while (running_) {
        auto clients = connections_.snapshot();
        if (tcpListeners_.empty() && clients.empty()) {
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
        pollfds.reserve(tcpListeners_.size() + clients.size());
        entries.reserve(tcpListeners_.size() + clients.size());

        for (const auto& listener : tcpListeners_) {
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
                while (running_) {
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

                    if (!setNonBlocking(clientSock)) {
                        CLOSE_SOCKET(clientSock);
                        continue;
                    }

                    ConnectionId id = connections_.add(clientSock, clientAddr);
                    LOG_INFO("NetworkIO", "TCP 연결 수락: id=" << id);
                }
                continue;
            }

            bool disconnected = (revents & (POLLHUP | POLLERR | POLLNVAL)) != 0;
            if (!disconnected && (revents & POLLRDNORM) != 0) {
                while (running_) {
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
                        inboundQueue_.push(std::move(packet));
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
            if (connections_.get(id, c)) {
                CLOSE_SOCKET(c.sock);
            }
            connections_.remove(id);
            LOG_INFO("NetworkIO", "TCP 연결 종료: id=" << id);
        }
    }
#else
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

    while (running_) {
        auto clients = connections_.snapshot();
        if (tcpListeners_.empty() && clients.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        for (const auto& listener : tcpListeners_) {
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

        const size_t desiredEventCount = (std::max<size_t>)(128, (tcpListeners_.size() + clients.size()) * 2);
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
                while (running_) {
                    sockaddr_in clientAddr{};
                    SockLenType addrLen = sizeof(clientAddr);
                    SocketType clientSock = accept(fd, reinterpret_cast<sockaddr*>(&clientAddr), &addrLen);
                    if (clientSock == INVALID_SOCK) {
                        int err = GetSocketPlatform().lastError();
                        if (err == ERR_WOULDBLOCK) break;
                        LOG_ERROR("NetworkIO", "TCP accept 실패: " << err);
                        break;
                    }

                    if (!setNonBlocking(clientSock)) {
                        CLOSE_SOCKET(clientSock);
                        continue;
                    }

                    ConnectionId id = connections_.add(clientSock, clientAddr);
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
                while (running_) {
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
                        inboundQueue_.push(std::move(packet));
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
            if (connections_.get(id, c)) {
                epoll_ctl(epollFd, EPOLL_CTL_DEL, c.sock, nullptr);
                clientFdToId.erase(c.sock);
                clientFdToAddr.erase(c.sock);
                CLOSE_SOCKET(c.sock);
            }
            connections_.remove(id);
            LOG_INFO("NetworkIO", "TCP 연결 종료: id=" << id);
        }
    }

    CLOSE_SOCKET(epollFd);
#endif
}

void NetworkIO::sendLoop() {
    NetPacket packet;
    while (outboundQueue_.pop(packet)) {
        if (packet.protocol == NetProtocol::Udp) {
            if (udpEndpoints_.empty()) continue;
            const auto& ep = udpEndpoints_.front();
            int sent = sendto(ep.sock, packet.data.data(),
                              static_cast<int>(packet.data.size()),
                              0, reinterpret_cast<const sockaddr*>(&packet.addr),
                              sizeof(packet.addr));
            if (sent == SOCK_ERROR) {
                LOG_WARN("NetworkIO", "UDP 전송 실패: " << GetSocketPlatform().lastError());
            }
        } else {
            ConnectionRegistry::TcpClient c{};
            if (!connections_.get(packet.connectionId, c)) {
                LOG_WARN("NetworkIO", "TCP 대상 없음: id=" << packet.connectionId);
                continue;
            }
            if (!sendAll(c.sock, packet.data.data(),
                         static_cast<int>(packet.data.size()))) {
                LOG_WARN("NetworkIO", "TCP 전송 실패: id=" << packet.connectionId);
            }
        }
    }
}

bool NetworkIO::ensurePlatform() {
    if (platformStarted_) {
        return true;
    }
    if (!GetSocketPlatform().initialize()) {
        LOG_ERROR("NetworkIO", "Socket platform init failed");
        return false;
    }
    platformStarted_ = true;
    return true;
}

bool NetworkIO::setNonBlocking(SocketType sock) {
    if (!GetSocketPlatform().setNonBlocking(sock)) {
        LOG_ERROR("NetworkIO", "Non-blocking 설정 실패: " << GetSocketPlatform().lastError());
        return false;
    }
    return true;
}

void NetworkIO::closeUdpSockets() {
    for (auto& ep : udpEndpoints_) {
        if (ep.sock != INVALID_SOCK) {
            CLOSE_SOCKET(ep.sock);
            ep.sock = INVALID_SOCK;
        }
    }
    udpEndpoints_.clear();
}

void NetworkIO::closeTcpSockets() {
    for (auto& listener : tcpListeners_) {
        if (listener.sock != INVALID_SOCK) {
            CLOSE_SOCKET(listener.sock);
            listener.sock = INVALID_SOCK;
        }
    }
    tcpListeners_.clear();

    connections_.closeAll();
}

void NetworkIO::closeAllSockets() {
    closeUdpSockets();
    closeTcpSockets();
}

bool NetworkIO::sendAll(SocketType sock, const char* data, int size) {
    int total = 0;
    while (total < size) {
        int sent = send(sock, data + total, size - total, 0);
        if (sent == SOCK_ERROR) {
            return false;
        }
        total += sent;
    }
    return true;
}
