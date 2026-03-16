#pragma once
#include "lobby/domain/Room.h"
#include <unordered_map>
#include <string>

class RoomManager {
private:
    std::unordered_map<std::string, Room> rooms;

public:
    Room& createRoom();
    bool roomExists(const std::string& roomCode) const;
    Room* getRoom(const std::string& roomCode);
    const std::unordered_map<std::string, Room>& getAllRooms() const { return rooms; }
    void removeRoomIfEmpty(const std::string& roomCode);
    bool isNameTakenGlobal(const std::string& name) const;
};
