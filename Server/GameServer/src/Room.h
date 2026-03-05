#pragma once
#include "Player.h"
#include <unordered_map>
#include <string>
#include <iostream>
#include <cstdlib>  // rand, srand 를 위해 추가
#include <ctime>    // time 을 위해 추가

enum class GamePhase { Waiting, InGame, GameOver };

class Room {
private:
    std::string roomCode;
    std::string hostId;
    GamePhase phase = GamePhase::Waiting;
    std::unordered_map<std::string, Player> players;

public:
    Room() = default;
    Room(std::string code) : roomCode(std::move(code)) {}

    const std::string& getRoomCode() const { return roomCode; }
    
    const std::string& getHostId() const { return hostId; }
    void setHostId(const std::string& host) { hostId = host; }
    
    GamePhase getPhase() const { return phase; }
    void setPhase(GamePhase newPhase) { phase = newPhase; }

    void addPlayer(const Player& player) {
        players[player.getName()] = player;
    }

    void removePlayer(const std::string& name) {
        players.erase(name);
    }

    Player* getPlayer(const std::string& name) {
        auto it = players.find(name);
        return (it != players.end()) ? &(it->second) : nullptr;
    }

    bool isEmpty() const { return players.empty(); }
    size_t getPlayerCount() const { return players.size(); }
    bool isNameTaken(const std::string& name) const { return players.count(name) > 0; }

    void broadcast(const json& message) const {
        for (const auto& [name, player] : players) {
            player.send(message);
        }
    }

    // 방장 제외 모두 레디 상태인지 확인
    bool allReady() const {
        if (players.empty()) return false;
        for (const auto& [name, player] : players) {
            if (name != hostId && !player.isReady()) return false;
        }
        return true;
    }

    // 방 코드 생성 함수
    static std::string GenerateRoomCode() {
        const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::string code;
        srand(static_cast<unsigned int>(time(nullptr))); // 경고 방지를 위해 캐스팅 추가
        for (int i = 0; i < 6; i++) {
            code += chars[rand() % chars.size()];
        }
        return code;
    }
};