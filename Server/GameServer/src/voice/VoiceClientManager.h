#pragma once

#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>

#include <string>
#include <unordered_map>
#include <chrono>
#include <cstdint>

#include "common/IClientManager.h"
#include "common/Logger.h"
#include "common/ScopedTimer.h"

struct ClientInfo {
    sockaddr_in addr;
    std::chrono::steady_clock::time_point lastSeen;
};

// IP:Port → uint64 key
inline uint64_t makeClientKey(const sockaddr_in& addr) {
    return (static_cast<uint64_t>(addr.sin_addr.s_addr) << 32)
         | ntohs(addr.sin_port);
}

class VoiceClientManager : public IClientManager<uint64_t, ClientInfo> {
private:

    // 전체 클라이언트
    std::unordered_map<uint64_t, ClientInfo> clients;

    // roomCode → speaker → clientKey
    std::unordered_map<std::string, std::unordered_map<std::string, uint64_t>> rooms;

    const int TIMEOUT_SECONDS = 300;

public:

    void addClient(const uint64_t& key, const ClientInfo& data) override {
        clients[key] = data;
    }

    void removeClient(const uint64_t& key) override {
        clients.erase(key);
    }

    bool hasClient(const uint64_t& key) const override {
        return clients.find(key) != clients.end();
    }

    size_t clientCount() const override {
        return clients.size();
    }

    void joinRoom(const std::string& roomCode,
                  const std::string& speaker,
                  const ClientInfo& clientInfo)
    {
        uint64_t key = makeClientKey(clientInfo.addr);

        clients[key] = clientInfo;
        rooms[roomCode][speaker] = key;
    }

    void leaveRoom(const std::string& roomCode,
                   const std::string& speaker)
    {
        if (rooms.count(roomCode)) {
            rooms[roomCode].erase(speaker);

            if (rooms[roomCode].empty())
                rooms.erase(roomCode);
        }
    }

    const std::unordered_map<uint64_t, ClientInfo>& getClients() const {
        return clients;
    }

    void cleanupStaleClients() override {

        PROFILE_SCOPE("Cleanup Stale Clients");

        auto now = std::chrono::steady_clock::now();

        // room cleanup
        for (auto itRoom = rooms.begin(); itRoom != rooms.end();) {

            auto& speakers = itRoom->second;

            for (auto itSpeaker = speakers.begin();
                 itSpeaker != speakers.end();) {

                uint64_t key = itSpeaker->second;

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
};