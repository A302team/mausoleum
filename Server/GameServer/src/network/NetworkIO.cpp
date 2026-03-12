#include "network/NetworkIO.h"
#include <algorithm>
#include <chrono>
#include "network/platform/SocketPlatform.h"

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
    endpoint.maxPacketSize = std::max(maxPacketSize, 256);
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
    while (running_) {
        if (udpEndpoints_.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        fd_set readfds;
        FD_ZERO(&readfds);
        SocketType maxfd = 0;
        for (const auto& ep : udpEndpoints_) {
            FD_SET(ep.sock, &readfds);
            if (ep.sock > maxfd) maxfd = ep.sock;
        }

        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms

        int ready = select(static_cast<int>(maxfd + 1), &readfds, nullptr, nullptr, &tv);
        if (ready == SOCK_ERROR) {
            int err = GetSocketPlatform().lastError();
            LOG_ERROR("NetworkIO", "UDP select 오류: " << err);
            continue;
        }
        if (ready == 0) continue;

        for (const auto& ep : udpEndpoints_) {
            if (!FD_ISSET(ep.sock, &readfds)) continue;

            std::vector<char> buffer;
            buffer.resize(ep.maxPacketSize);

            sockaddr_in sender{};
            SockLenType addrLen = sizeof(sender);
            int bytesRecv = recvfrom(ep.sock, buffer.data(), static_cast<int>(buffer.size()),
                                     0, reinterpret_cast<sockaddr*>(&sender), &addrLen);
            if (bytesRecv == SOCK_ERROR) {
                int err = GetSocketPlatform().lastError();
                if (err == ERR_CONNRESET || err == ERR_WOULDBLOCK) continue;
                LOG_ERROR("NetworkIO", "UDP 수신 실패: " << err);
                continue;
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

void NetworkIO::tcpLoop() {
    while (running_) {
        if (tcpListeners_.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        fd_set readfds;
        FD_ZERO(&readfds);
        SocketType maxfd = 0;

        for (const auto& listener : tcpListeners_) {
            FD_SET(listener.sock, &readfds);
            if (listener.sock > maxfd) maxfd = listener.sock;
        }

        auto clients = connections_.snapshot();
        for (const auto& client : clients) {
            FD_SET(client.sock, &readfds);
            if (client.sock > maxfd) maxfd = client.sock;
        }

        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 100000;

        int ready = select(static_cast<int>(maxfd + 1), &readfds, nullptr, nullptr, &tv);
        if (ready == SOCK_ERROR) {
            int err = GetSocketPlatform().lastError();
            LOG_ERROR("NetworkIO", "TCP select 오류: " << err);
            continue;
        }
        if (ready == 0) continue;

        for (const auto& listener : tcpListeners_) {
            if (!FD_ISSET(listener.sock, &readfds)) continue;

            sockaddr_in clientAddr{};
            SockLenType addrLen = sizeof(clientAddr);
            SocketType clientSock = accept(listener.sock,
                                           reinterpret_cast<sockaddr*>(&clientAddr),
                                           &addrLen);
            if (clientSock == INVALID_SOCK) {
                int err = GetSocketPlatform().lastError();
                if (err == ERR_WOULDBLOCK) continue;
                LOG_ERROR("NetworkIO", "TCP accept 실패: " << err);
                continue;
            }

            if (!setNonBlocking(clientSock)) {
                CLOSE_SOCKET(clientSock);
                continue;
            }

            ConnectionId id = connections_.add(clientSock, clientAddr);
            LOG_INFO("NetworkIO", "TCP 연결 수락: id=" << id);
        }

        std::vector<ConnectionId> toRemove;
        for (const auto& client : clients) {
            if (!FD_ISSET(client.sock, &readfds)) continue;

            std::vector<char> buffer;
            buffer.resize(4096);
            int bytesRecv = recv(client.sock, buffer.data(), static_cast<int>(buffer.size()), 0);
            if (bytesRecv <= 0) {
                toRemove.push_back(client.id);
                continue;
            }

            buffer.resize(bytesRecv);
            NetPacket packet;
            packet.protocol = NetProtocol::Tcp;
            packet.connectionId = client.id;
            packet.addr = client.addr;
            packet.data = std::move(buffer);
            inboundQueue_.push(std::move(packet));
        }

        for (ConnectionId id : toRemove) {
            ConnectionRegistry::TcpClient c{};
            if (connections_.get(id, c)) {
                CLOSE_SOCKET(c.sock);
            }
            connections_.remove(id);
            LOG_INFO("NetworkIO", "TCP 연결 종료: id=" << id);
        }
    }
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
