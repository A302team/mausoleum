#include "network/NetworkIO.h"
#include <algorithm>
#include "network/platform/INetworkEventLoop.h"
#include "network/platform/SocketPlatform.h"

NetworkIO::NetworkIO(ConnectionRegistry& connections,
                     ConcurrentQueue<NetPacket>& inbound,
                     ConcurrentQueue<NetPacket>& outbound)
    : connections_(connections),
      inboundQueue_(inbound),
      outboundQueue_(outbound),
      eventLoop_(CreateNetworkEventLoop()) {}

NetworkIO::~NetworkIO() {
    stop();
}

bool NetworkIO::start() {
    if (running_.exchange(true)) {
        return true;
    }

    if (!eventLoop_) {
        LOG_ERROR("NetworkIO", "지원되지 않는 플랫폼입니다. Network event loop 생성 실패");
        running_ = false;
        return false;
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

bool NetworkIO::configureEndpoints(const std::vector<NetworkUdpEndpointConfig>& udpConfigs,
                                   const std::vector<NetworkTcpListenerConfig>& tcpConfigs) {
    if (running_) {
        LOG_ERROR("NetworkIO", "실행 중에는 엔드포인트를 재구성할 수 없습니다.");
        return false;
    }

    closeAllSockets();

    for (const auto& cfg : udpConfigs) {
        if (!registerUdpEndpoint(cfg.port, cfg.maxPacketSize)) {
            closeAllSockets();
            return false;
        }
    }

    for (const auto& cfg : tcpConfigs) {
        if (!registerTcpListener(cfg.port)) {
            closeAllSockets();
            return false;
        }
    }

    return true;
}

bool NetworkIO::registerUdpEndpoint(int port, int maxPacketSize) {
    if (!ensurePlatform()) {
        return false;
    }
    SocketType sock = GetSocketPlatform().createUdpEndpoint(port);
    if (sock == INVALID_SOCK) {
        LOG_ERROR("NetworkIO", "UDP 엔드포인트 생성 실패: " << GetSocketPlatform().lastError());
        return false;
    }

    NetworkUdpEndpoint endpoint;
    endpoint.sock = sock;
    endpoint.port = port;
    endpoint.maxPacketSize = (std::max)(maxPacketSize, 256);
    udpEndpoints_.push_back(endpoint);
    LOG_INFO("NetworkIO", "UDP 엔드포인트 등록: " << port);
    return true;
}

bool NetworkIO::registerTcpListener(int port) {
    if (!ensurePlatform()) {
        return false;
    }
    SocketType sock = GetSocketPlatform().createTcpListener(port);
    if (sock == INVALID_SOCK) {
        LOG_ERROR("NetworkIO", "TCP 리스너 생성 실패: " << GetSocketPlatform().lastError());
        return false;
    }

    NetworkTcpListener listener;
    listener.sock = sock;
    listener.port = port;
    tcpListeners_.push_back(listener);
    LOG_INFO("NetworkIO", "TCP 리스너 등록: " << port);
    return true;
}

void NetworkIO::udpLoop() {
    const auto udpSnapshot = udpEndpoints_;
    eventLoop_->udpLoop(running_, udpSnapshot, inboundQueue_);
}

void NetworkIO::tcpLoop() {
    const auto listenerSnapshot = tcpListeners_;
    eventLoop_->tcpLoop(running_, listenerSnapshot, connections_, inboundQueue_);
}

void NetworkIO::sendLoop() {
    NetPacket packet;
    while (outboundQueue_.pop(packet)) {
        const char* payload = packet.payloadData();
        const size_t payloadSize = packet.payloadSize();
        if (payloadSize == 0) {
            continue;
        }

        if (packet.protocol == NetProtocol::Udp) {
            if (udpEndpoints_.empty()) continue;
            const auto& ep = udpEndpoints_.front();
            int sent = sendto(ep.sock, payload,
                              static_cast<int>(payloadSize),
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
            if (!sendAll(c.sock, payload,
                         static_cast<int>(payloadSize))) {
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

void NetworkIO::closeUdpSockets() {
    for (auto& ep : udpEndpoints_) {
        if (ep.sock != INVALID_SOCK && !GetSocketPlatform().closeSocket(ep.sock)) {
            LOG_WARN("NetworkIO", "UDP 소켓 종료 실패: " << GetSocketPlatform().lastError());
            ep.sock = INVALID_SOCK;
        }
    }
    udpEndpoints_.clear();
}

void NetworkIO::closeTcpSockets() {
    for (auto& listener : tcpListeners_) {
        if (listener.sock != INVALID_SOCK && !GetSocketPlatform().closeSocket(listener.sock)) {
            LOG_WARN("NetworkIO", "TCP 리스너 종료 실패: " << GetSocketPlatform().lastError());
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
