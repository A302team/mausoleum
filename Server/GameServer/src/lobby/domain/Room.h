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

    void addPlayer(const Player& player);
    void removePlayer(const std::string& name);
    Player* getPlayer(const std::string& name);

    bool isEmpty() const { return players.empty(); }
    size_t getPlayerCount() const { return players.size(); }
    bool isNameTaken(const std::string& name) const { return players.count(name) > 0; }

    void broadcast(const json& message) const;
    void broadcastExcept(const std::string& exceptName, const json& message);

    bool allReady() const;

    static std::string GenerateRoomCode();
};