#include "voice/VoiceClientManager.h"

CLIENT_KEY makeClientKey(const sockaddr_in& addr) {
    return (static_cast<CLIENT_KEY>(addr.sin_addr.s_addr) << 32)
         | ntohs(addr.sin_port);
}

void VoiceClientManager::addClient(const CLIENT_KEY& key, const ClientInfo& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    clients[key] = data;
}

void VoiceClientManager::removeClient(const CLIENT_KEY& key) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto roomCodeIt = clientRooms.find(key);
    if (roomCodeIt != clientRooms.end()) {
        auto roomIt = rooms.find(roomCodeIt->second);
        if (roomIt != rooms.end()) {
            roomIt->second.erase(key);
            if (roomIt->second.empty()) {
                rooms.erase(roomIt);
            }
        }
        clientRooms.erase(roomCodeIt);
    }

    clients.erase(key);
}

bool VoiceClientManager::hasClient(const CLIENT_KEY& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return clients.find(key) != clients.end();
}

size_t VoiceClientManager::clientCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return clients.size();
}

void VoiceClientManager::joinRoom(const std::string& roomCode,
                                  const CLIENT_KEY& key,
                                  const ClientInfo& clientInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    clients[key] = clientInfo;

    auto previousRoomIt = clientRooms.find(key);
    if (previousRoomIt != clientRooms.end() && previousRoomIt->second != roomCode) {
        auto oldRoomIt = rooms.find(previousRoomIt->second);
        if (oldRoomIt != rooms.end()) {
            oldRoomIt->second.erase(key);
            if (oldRoomIt->second.empty()) {
                rooms.erase(oldRoomIt);
            }
        }
    }

    rooms[roomCode].insert(key);
    clientRooms[key] = roomCode;
}

void VoiceClientManager::leaveRoom(const CLIENT_KEY& key)
{
    std::lock_guard<std::mutex> lock(mutex_);

    auto roomCodeIt = clientRooms.find(key);
    if (roomCodeIt == clientRooms.end()) {
        return;
    }

    auto roomIt = rooms.find(roomCodeIt->second);
    if (roomIt != rooms.end()) {
        roomIt->second.erase(key);

        if (roomIt->second.empty()) {
            rooms.erase(roomIt);
        }
    }

    clientRooms.erase(roomCodeIt);
}

std::vector<std::pair<CLIENT_KEY, ClientInfo>> VoiceClientManager::getClientsSnapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<CLIENT_KEY, ClientInfo>> snapshot;
    snapshot.reserve(clients.size());
    for (const auto& kv : clients) {
        snapshot.push_back(kv);
    }
    return snapshot;
}

std::vector<std::pair<CLIENT_KEY, ClientInfo>> VoiceClientManager::getRoomClientsSnapshot(const std::string& roomCode) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<CLIENT_KEY, ClientInfo>> snapshot;

    auto roomIt = rooms.find(roomCode);
    if (roomIt == rooms.end()) {
        return snapshot;
    }

    snapshot.reserve(roomIt->second.size());
    for (const CLIENT_KEY clientKey : roomIt->second) {
        auto clientIt = clients.find(clientKey);
        if (clientIt != clients.end()) {
            snapshot.emplace_back(clientIt->first, clientIt->second);
        }
    }

    return snapshot;
}

void VoiceClientManager::cleanupStaleClients() {

    PROFILE_SCOPE("Cleanup Stale Clients");

    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();

    // room cleanup
    for (auto itRoom = rooms.begin(); itRoom != rooms.end();) {

        auto& members = itRoom->second;

        for (auto itMember = members.begin();
             itMember != members.end();) {

            CLIENT_KEY key = *itMember;

            auto clientIt = clients.find(key);
            if (clientIt == clients.end()) {
                clientRooms.erase(key);
                itMember = members.erase(itMember);
                continue;
            }

            auto& client = clientIt->second;

            auto duration =
                std::chrono::duration_cast<std::chrono::seconds>(
                    now - client.lastSeen).count();

            if (duration > TIMEOUT_SECONDS) {

                LOG_INFO("VoiceClientManager",
                    "방 타임아웃: " << itRoom->first
                    << " / clientKey: " << key);

                clientRooms.erase(key);
                itMember = members.erase(itMember);
            }
            else {
                ++itMember;
            }
        }

        if (members.empty()) {

            LOG_INFO("VoiceClientManager",
                "방 제거: " << itRoom->first);

            itRoom = rooms.erase(itRoom);
        }
        else {
            ++itRoom;
        }
    }

    // client cleanup
    for (auto it = clients.begin(); it != clients.end();) {

        auto duration =
            std::chrono::duration_cast<std::chrono::seconds>(
                now - it->second.lastSeen).count();

        if (duration > TIMEOUT_SECONDS) {

            char ip[INET_ADDRSTRLEN];

            inet_ntop(AF_INET,
                      &it->second.addr.sin_addr,
                      ip,
                      sizeof(ip));

            LOG_INFO("VoiceClientManager",
                "클라이언트 타임아웃: "
                << ip << ":"
                << ntohs(it->second.addr.sin_port));

            auto roomCodeIt = clientRooms.find(it->first);
            if (roomCodeIt != clientRooms.end()) {
                auto roomIt = rooms.find(roomCodeIt->second);
                if (roomIt != rooms.end()) {
                    roomIt->second.erase(it->first);
                    if (roomIt->second.empty()) {
                        rooms.erase(roomIt);
                    }
                }
                clientRooms.erase(roomCodeIt);
            }

            it = clients.erase(it);
        }
        else {
            ++it;
        }
    }
}
