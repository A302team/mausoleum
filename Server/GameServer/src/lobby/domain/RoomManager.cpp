#include "lobby/domain/RoomManager.h"

Room& RoomManager::createRoom() {
    std::string roomCode;
    do {
        roomCode = Room::GenerateRoomCode();
    } while (rooms.count(roomCode));

    rooms[roomCode] = Room(roomCode);
    return rooms[roomCode];
}

bool RoomManager::roomExists(const std::string& roomCode) const {
    return rooms.count(roomCode) > 0;
}

Room* RoomManager::getRoom(const std::string& roomCode) {
    auto it = rooms.find(roomCode);
    return (it != rooms.end()) ? &(it->second) : nullptr;
}

void RoomManager::removeRoom(const std::string& roomCode) {
    rooms.erase(roomCode);
}

void RoomManager::removeRoomIfEmpty(const std::string& roomCode) {
    if (rooms.count(roomCode) && rooms.at(roomCode).isEmpty()) {
        rooms.erase(roomCode);
    }
}

bool RoomManager::isNameTakenGlobal(const std::string& name) const {
    for (const auto& [roomCode, room] : rooms) {
        if (room.isNameTaken(name)) return true;
    }
    return false;
}
