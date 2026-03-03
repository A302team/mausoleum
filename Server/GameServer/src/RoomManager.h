#pragma once
#include "Room.h"
#include <unordered_map>

class RoomManager
{
public:
    std::unordered_map<std::string, Room> rooms;

    Room &createRoom()
    {
        std::string roomCode;
        do
        {
            roomCode = Room::GenerateRoomCode();
        } while (rooms.count(roomCode));

        Room room;
        room.roomId = roomCode;
        rooms[roomCode] = room;
        return rooms[roomCode];
    }

    bool roomExists(const std::string &roomCode)
    {
        return rooms.count(roomCode) > 0;
    }

    void removeRoomIfEmpty(const std::string &roomId)
    {
        if (rooms.count(roomId) && rooms[roomId].isEmpty())
        {
            rooms.erase(roomId);
        }
    }

    // ws로 플레이어가 어느 방에 있는지 찾기
    // 연결 끊겼을 때 필요
    std::pair<std::string, std::string> findPlayerByWs(
        uWS::WebSocket<false, true, int> *ws)
    {
        for (auto &[roomId, room] : rooms)
        {
            for (auto &[name, player] : room.players)
            {
                if (player.ws == ws)
                {
                    return {roomId, name};
                }
            }
        }
        return {"", ""};
    }

    bool isNameTakenGlobal(const std::string& name)
    {
        for(auto& [roomId, room] : rooms)
        {
            if(room.isNameTaken(name)) return true;
        }
        return false;
    }
};