#pragma once
#include <mutex>
#include <unordered_map>
#include <vector>
#include "network/NetworkTypes.h"

class ConnectionRegistry {
public:
    struct TcpClient {
        SocketType sock = INVALID_SOCK;
        sockaddr_in addr{};
        ConnectionId id = 0;
    };

    ConnectionId add(SocketType sock, const sockaddr_in& addr);
    bool remove(ConnectionId id);
    bool get(ConnectionId id, TcpClient& out) const;
    std::vector<TcpClient> snapshot() const;
    void closeAll();

private:
    mutable std::mutex mutex_;
    std::unordered_map<ConnectionId, TcpClient> clients_;
    ConnectionId nextId_ = 1;
};
