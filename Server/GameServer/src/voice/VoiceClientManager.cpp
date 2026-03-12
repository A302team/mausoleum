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
                                  const std::string& speaker,
                                  const ClientInfo& clientInfo)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CLIENT_KEY key = makeClientKey(clientInfo.addr);

    clients[key] = clientInfo;
    rooms[roomCode][speaker] = key;
}

void VoiceClientManager::leaveRoom(const std::string& roomCode,
                                   const std::string& speaker)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (rooms.count(roomCode)) {
        rooms[roomCode].erase(speaker);

        if (rooms[roomCode].empty())
            rooms.erase(roomCode);
    }
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

void VoiceClientManager::cleanupStaleClients() {

    PROFILE_SCOPE("Cleanup Stale Clients");

    std::lock_guard<std::mutex> lock(mutex_);
    auto now = std::chrono::steady_clock::now();

    // room cleanup
    for (auto itRoom = rooms.begin(); itRoom != rooms.end();) {

        auto& speakers = itRoom->second;

        for (auto itSpeaker = speakers.begin();
             itSpeaker != speakers.end();) {

            CLIENT_KEY key = itSpeaker->second;

            if (!clients.count(key)) {
                itSpeaker = speakers.erase(itSpeaker);
                continue;
            }

            auto& client = clients[key];

            auto duration =
                std::chrono::duration_cast<std::chrono::seconds>(
                    now - client.lastSeen).count();

            if (duration > TIMEOUT_SECONDS) {

                LOG_INFO("VoiceClientManager",
                    "방 타임아웃: " << itRoom->first
                    << " / 화자: " << itSpeaker->first);

                itSpeaker = speakers.erase(itSpeaker);
            }
            else {
                ++itSpeaker;
            }
        }

        if (speakers.empty()) {

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

            it = clients.erase(it);
        }
        else {
            ++it;
        }
    }
}
