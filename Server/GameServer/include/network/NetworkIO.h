#pragma once
#include <atomic>
#include <thread>
#include <vector>
#include "common/ConcurrentQueue.h"
#include "common/Platform.h"
#include "common/logging/Logger.h"
#include "network/ConnectionRegistry.h"
#include "network/NetworkTypes.h"

class NetworkIO {
public:
    NetworkIO(ConnectionRegistry& connections,
              ConcurrentQueue<NetPacket>& inbound,
              ConcurrentQueue<NetPacket>& outbound);
    ~NetworkIO();

    bool start();
    void stop();

    bool addUdpEndpoint(int port, int maxPacketSize = 2048);
    bool addTcpListener(int port);

private:
    struct UdpEndpoint {
        SocketType sock = INVALID_SOCK;
        int port = 0;
        int maxPacketSize = 2048;
    };

    struct TcpListener {
        SocketType sock = INVALID_SOCK;
        int port = 0;
    };

    ConnectionRegistry& connections_;
    ConcurrentQueue<NetPacket>& inboundQueue_;
    ConcurrentQueue<NetPacket>& outboundQueue_;

    std::vector<UdpEndpoint> udpEndpoints_;
    std::vector<TcpListener> tcpListeners_;

    std::thread udpThread_;
    std::thread tcpThread_;
    std::thread sendThread_;
    std::atomic<bool> running_{false};

    bool platformStarted_ = false;

    void udpLoop();
    void tcpLoop();
    void sendLoop();

    bool ensurePlatform();
    bool setNonBlocking(SocketType sock);
    void closeUdpSockets();
    void closeTcpSockets();
    void closeAllSockets();

    bool sendAll(SocketType sock, const char* data, int size);
};
