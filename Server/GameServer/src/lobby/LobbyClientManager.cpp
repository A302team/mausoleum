#include "LobbyClientManager.h"

void LobbyClientManager::addClient(WebSocketType* const& key, const LobbyClientInfo& data) {
    clients[key] = data;
}

void LobbyClientManager::removeClient(WebSocketType* const& key) {
    clients.erase(key);
}

bool LobbyClientManager::hasClient(WebSocketType* const& key) const {
    return clients.count(key) > 0;
}

size_t LobbyClientManager::clientCount() const {
    return clients.size();
}

void LobbyClientManager::cleanupStaleClients() {
    // WebSocket은 onDisconnect 이벤트로 관리되므로 스케줄링 기반 Stale 제거 불필요
}

LobbyClientInfo* LobbyClientManager::getClientInfo(WebSocketType* ws) {
    auto it = clients.find(ws);
    if (it != clients.end()) return &it->second;
    return nullptr;
}
