#pragma once
#include "Player.h"
#include <unordered_map>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

enum class GamePhase
{
    Waiting,
    InGame,
    GameOver
};

class Room
{
public:
    std::string roomId;
    std::string hostId;
    GamePhase phase = GamePhase::Waiting;
    std::unordered_map<std::string, Player> players;

    void addPlayer(Player player)
    {
        players[player.name] = player;
    }

    void removePlayer(const std::string &name)
    {
        players.erase(name);
    }

    // 전체 브로드캐스트
    void broadcast(const json &message)
    {
        std::string msg = message.dump();
        for (auto &[name, player] : players)
        {
            if (player.ws)
            {
                player.ws->send(msg, uWS::OpCode::TEXT);
            }
        }
    }

    // 특정 플레이어에게만
    void sendTo(const std::string &name, const json &message)
    {
        if (players.count(name) && players[name].ws)
        {
            players[name].ws->send(message.dump(), uWS::OpCode::TEXT);
        }
    }

    bool isEmpty()
    {
        return players.empty();
    }

    // 모두 레디인지 (방장 제외)
    bool allReady()
    {
        for (auto &[name, player] : players)
        {
            if (name == hostId)
                continue;
            if (!player.isReady)
                return false;
        }
        return !players.empty();
    }

    // 생존자 수
    int aliveCount()
    {
        int count = 0;
        for (auto &[name, player] : players)
        {
            if (player.isAlive)
                count++;
        }
        return count;
    }

    bool isNameTaken(const std::string &name)
    {
        return players.count(name) > 0;
    }

    static std::string GenerateRoomCode()
    {
        const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::string code;
        srand(time(nullptr));
        for (int i = 0; i < 6; i++)
        {
            code += chars[rand() % chars.size()];
        }

        return code;
    }
};