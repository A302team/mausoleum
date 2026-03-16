#include "lobby/domain/Room.h"

void Room::addPlayer(const Player &player)
{
    players[player.getName()] = player;
}

bool Room::removePlayer(const std::string &name)
{
    players.erase(name);
    if (!players.empty() && hostId == name)
    {
        // 남은 사람 중 랜덤(해시맵의 첫 원소) 한 명을 즉시 새 방장으로 지정!
        hostId = players.begin()->first;
        return true;
    }
    return false;
}

Player *Room::getPlayer(const std::string &name)
{
    auto it = players.find(name);
    return (it != players.end()) ? &(it->second) : nullptr;
}

void Room::broadcast(const json &message) const
{
    for (const auto &[name, player] : players)
    {
        player.send(message);
    }
}

void Room::broadcastExcept(const std::string &exceptName, const json &message)
{
    for (auto &[name, player] : players)
    {
        if (name != exceptName)
        {
            player.send(message);
        }
    }
}

bool Room::allReady() const
{
    if (players.empty())
        return false;
    for (const auto &[name, player] : players)
    {
        if (name != hostId && !player.isReady())
            return false;
    }
    return true;
}

std::string Room::GenerateRoomCode()
{
    const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string code;
    srand(static_cast<unsigned int>(time(nullptr)));
    for (int i = 0; i < 6; i++)
    {
        code += chars[rand() % chars.size()];
    }
    return code;
}
