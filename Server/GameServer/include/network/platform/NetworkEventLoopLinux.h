#pragma once
#if defined(__linux__)
#include "network/platform/INetworkEventLoop.h"

class LinuxNetworkEventLoop final : public INetworkEventLoop {
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
