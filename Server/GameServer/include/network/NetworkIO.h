#pragma once
#include <atomic>
#include <memory>
#include <thread>
#include <vector>
#include "common/ConcurrentQueue.h"
#include "common/Platform.h"
#include "common/logging/Logger.h"
#include "network/ConnectionRegistry.h"
#include "network/NetworkIOPrimitives.h"
#include "network/NetworkTypes.h"

class INetworkEventLoop;

class NetworkIO {
public:
    NetworkIO(ConnectionRegistry& connections,
              ConcurrentQueue<NetPacket>& inbound,
              ConcurrentQueue<NetPacket>& outbound);
    ~NetworkIO();

    bool start();
    void stop();

    bool configureEndpoints(const std::vector<NetworkUdpEndpointConfig>& udpConfigs,
                            const std::vector<NetworkTcpListenerConfig>& tcpConfigs);

private:
    ConnectionRegistry& connections_;
    ConcurrentQueue<NetPacket>& inboundQueue_;
    ConcurrentQueue<NetPacket>& outboundQueue_;
    std::unique_ptr<INetworkEventLoop> eventLoop_;

    std::vector<NetworkUdpEndpoint> udpEndpoints_;
    std::vector<NetworkTcpListener> tcpListeners_;

    std::thread udpThread_;
    std::thread tcpThread_;
    std::thread sendThread_;
    std::atomic<bool> running_{false};

    bool platformStarted_ = false;

    void udpLoop();
    void tcpLoop();
    void sendLoop();

    bool ensurePlatform();
    bool registerUdpEndpoint(int port, int maxPacketSize);
    bool registerTcpListener(int port);
    void closeUdpSockets();
    void closeTcpSockets();
    void closeAllSockets();

    bool sendAll(SocketType sock, const char* data, int size);
};
