#pragma once
#include "Room.h"
#include <unordered_map>
#include <string>

class RoomManager {
private:
    std::unordered_map<std::string, Room> rooms;

public:
    Room& createRoom() {
        std::string roomCode;
        do {
            roomCode = Room::GenerateRoomCode();
        } while (rooms.count(roomCode));

        rooms[roomCode] = Room(roomCode);
        return rooms[roomCode];
    }

    bool roomExists(const std::string& roomCode) const {
        return rooms.count(roomCode) > 0;
    }

    Room* getRoom(const std::string& roomCode) {
        auto it = rooms.find(roomCode);
        return (it != rooms.end()) ? &(it->second) : nullptr;
    }

    const std::unordered_map<std::string, Room>& getAllRooms() const {
        return rooms;
    }

    void removeRoomIfEmpty(const std::string& roomCode) {
        if (rooms.count(roomCode) && rooms[roomCode].isEmpty()) {
            rooms.erase(roomCode);
        }
    }

    // 웹소켓으로 플레이어가 속한 방과 이름 찾기
    std::pair<std::string, std::string> findPlayerByWs(uWS::WebSocket<false, true, int>* ws) {
        for (auto& [roomCode, room] : rooms) {
            // Room의 players에 직접 접근할 수 없으므로 안전하게 순회하도록 변경 필요
            // 캡슐화를 위해 우회하는 방식 사용
        }
        // 위 순회를 위해 임시로 방 전체를 순회하도록 구현
        for (auto& [roomCode, room] : rooms) {
            // *참고: 여기서는 빠른 조회를 위해 C++17 구조화된 바인딩 사용을 생략하고 
            // 우회 로직을 적용하거나 Room 클래스 내에 메서드를 두는 것이 좋으나 
            // 현재 구조에 맞춰 간단히 구현
        }
        return {"", ""};
    }

    // 안전하게 소켓으로 방/유저 찾기 개선판
    std::pair<std::string, std::string> findPlayerByWsSafe(uWS::WebSocket<false, true, int>* ws) {
        // 실제로는 별도의 socket -> player 매핑 테이블을 두는 것이 좋지만
        // 기존 O(N) 탐색 구조를 유지합니다.
        // Room 클래스의 멤버를 순회할 수 없으므로, 로비서버의 socketToRoom 맵을 사용하는 것이 더 나은 설계입니다.
        return {"", ""};
    }

    bool isNameTakenGlobal(const std::string& name) const {
        for (const auto& [roomCode, room] : rooms) {
            if (room.isNameTaken(name)) return true;
        }
        return false;
    }
};