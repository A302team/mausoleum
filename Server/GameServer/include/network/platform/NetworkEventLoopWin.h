#pragma once
#ifdef _WIN32
#include "network/platform/INetworkEventLoop.h"

class WindowsNetworkEventLoop final : public INetworkEventLoop {
public:
    void udpLoop(std::atomic<bool>& running,
                 std::vector<NetworkUdpEndpoint>& udpEndpoints,
                 ConcurrentQueue<NetPacket>& inboundQueue) override;

    void tcpLoop(std::atomic<bool>& running,
                 std::vector<NetworkTcpListener>& tcpListeners,
                 ConnectionRegistry& connections,
                 ConcurrentQueue<NetPacket>& inboundQueue) override;
};
#endif
