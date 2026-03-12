#include "network/ConnectionRegistry.h"

ConnectionId ConnectionRegistry::add(SocketType sock, const sockaddr_in& addr) {
    std::lock_guard<std::mutex> lock(mutex_);
    ConnectionId id = nextId_++;
    clients_[id] = TcpClient{sock, addr, id};
    return id;
}

bool ConnectionRegistry::remove(ConnectionId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return clients_.erase(id) > 0;
}

bool ConnectionRegistry::get(ConnectionId id, TcpClient& out) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = clients_.find(id);
    if (it == clients_.end()) return false;
    out = it->second;
    return true;
}

std::vector<ConnectionRegistry::TcpClient> ConnectionRegistry::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<TcpClient> list;
    list.reserve(clients_.size());
    for (const auto& kv : clients_) {
        list.push_back(kv.second);
    }
    return list;
}

void ConnectionRegistry::closeAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& kv : clients_) {
        if (kv.second.sock != INVALID_SOCK) {
            CLOSE_SOCKET(kv.second.sock);
            kv.second.sock = INVALID_SOCK;
        }
    }
    clients_.clear();
}
