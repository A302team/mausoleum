#pragma once
#include <atomic>
#include <memory>
#include <vector>
#include "common/ConcurrentQueue.h"
#include "network/ConnectionRegistry.h"
#include "network/NetworkIOPrimitives.h"
#include "network/NetworkTypes.h"

class INetworkEventLoop {
public:
    virtual ~INetworkEventLoop() = default;

    virtual void udpLoop(std::atomic<bool>& running,
                         std::vector<NetworkUdpEndpoint>& udpEndpoints,
                         ConcurrentQueue<NetPacket>& inboundQueue) = 0;

    virtual void tcpLoop(std::atomic<bool>& running,
                         std::vector<NetworkTcpListener>& tcpListeners,
                         ConnectionRegistry& connections,
                         ConcurrentQueue<NetPacket>& inboundQueue) = 0;
};

std::unique_ptr<INetworkEventLoop> CreateNetworkEventLoop();
