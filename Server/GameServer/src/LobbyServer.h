#pragma once
#include <uwebsockets/App.h>
#include <nlohmann/json.hpp>
#include <functional>
#include <unordered_map>
#include <iostream>
#include "RoomManager.h"

using json = nlohmann::json;
using WebSocketType = uWS::WebSocket<false, true, int>;

class LobbyServer {
private:
    RoomManager roomManager;
    
    // 소켓으로 유저 정보를 빠르게 찾기 위한 매핑 테이블
    struct ClientInfo { std::string roomCode; std::string playerName; };
    std::unordered_map<WebSocketType*, ClientInfo> socketMap;

    using MessageHandler = std::function<void(WebSocketType*, const json&)>;
    std::unordered_map<std::string, MessageHandler> handlers;

public:
    LobbyServer() {
        registerHandlers();
    }

    void run(int port) {
        uWS::App().ws<int>("/*", {
            .open = [this](auto* ws) { std::cout << "[Lobby] 클라이언트 접속!" << std::endl; },
            .message = [this](auto* ws, std::string_view msg, uWS::OpCode opCode) {
                this->onMessage(ws, msg);
            },
            .close = [this](auto* ws, int, std::string_view) {
                this->onDisconnect(ws);
            }
        }).listen(port, [port](auto* token) {
            if (token) std::cout << "[Lobby] 서버 시작! 포트: " << port << std::endl;
        }).run();
    }

private:
    void registerHandlers() {
        handlers["create_room"]    = [this](auto* ws, const json& data) { handleCreateRoom(ws, data); };
        handlers["join_room"]      = [this](auto* ws, const json& data) { handleJoinRoom(ws, data); };
        handlers["ready"]          = [this](auto* ws, const json& data) { handleReady(ws, data); };
        handlers["start_game"]     = [this](auto* ws, const json& data) { handleStartGame(ws, data); };
        handlers["get_room_list"]  = [this](auto* ws, const json& data) { handleGetRoomList(ws, data); };
        handlers["check_nickname"] = [this](auto* ws, const json& data) { handleCheckNickname(ws, data); };
        handlers["leave_room"]     = [this](auto* ws, const json& data) { handleLeaveRoom(ws, data); };
    }

    void onMessage(WebSocketType* ws, std::string_view msg) {
        try {
            json received = json::parse(msg);
            std::string type = received["type"];
            json data = received["data"];

            auto it = handlers.find(type);
            if (it != handlers.end()) {
                it->second(ws, data);
            } else {
                std::cout << "[Lobby] 알 수 없는 메시지 타입: " << type << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "[Lobby] JSON 파싱 오류: " << e.what() << std::endl;
        }
    }

    void handleCreateRoom(WebSocketType* ws, const json& data) {
        std::string playerName = data["playerName"];

        if (roomManager.isNameTakenGlobal(playerName)) {
            sendError(ws, "이미 사용 중인 닉네임입니다.");
            return;
        }

        Room& room = roomManager.createRoom();
        room.setHostId(playerName);
        room.addPlayer(Player(playerName, ws));
        socketMap[ws] = {room.getRoomCode(), playerName}; // 매핑 저장

        std::cout << "[" << room.getRoomCode() << "] " << playerName << " 님 방 생성 (방장)" << std::endl;

        ws->send(json({
            {"type", "room_created"},
            {"data", {{"roomCode", room.getRoomCode()}, {"playerName", playerName}, {"isHost", true}}}
        }).dump(), uWS::OpCode::TEXT);
    }

    void handleJoinRoom(WebSocketType* ws, const json& data) {
        std::string roomCode = data["roomCode"];
        std::string playerName = data["playerName"];

        if (!roomManager.roomExists(roomCode)) {
            sendError(ws, "존재하지 않는 코드입니다.");
            return;
        }

        if (roomManager.isNameTakenGlobal(playerName)) {
            sendError(ws, "이미 사용 중인 닉네임입니다.");
            return;
        }

        Room* room = roomManager.getRoom(roomCode);
        if (room) {
            room->addPlayer(Player(playerName, ws));
            socketMap[ws] = {roomCode, playerName}; // 매핑 저장
            
            std::cout << "[" << roomCode << "] " << playerName << " 님 입장" << std::endl;

            ws->send(json({
                {"type", "room_joined"},
                {"data", {{"roomCode", roomCode}, {"playerName", playerName}, {"isHost", room->getHostId() == playerName}}}
            }).dump(), uWS::OpCode::TEXT);

            room->broadcast({
                {"type", "player_entered"},
                {"data", {{"playerName", playerName}}}
            });
        }
    }

    void handleReady(WebSocketType* ws, const json& data) {
        std::string roomCode = data["roomCode"];
        std::string playerName = data["playerName"];

        Room* room = roomManager.getRoom(roomCode);
        if (room) {
            Player* player = room->getPlayer(playerName);
            if (player) {
                player->setReady(true);
                std::cout << "[" << roomCode << "] " << playerName << " 님 레디!" << std::endl;

                room->broadcast({
                    {"type", "player_ready"},
                    {"data", {{"playerName", playerName}}}
                });
            }
        }
    }
    
    void handleStartGame(WebSocketType* ws, const json& data) {
        std::string roomCode = data["roomCode"];
        std::string playerName = data["playerName"];

        Room* room = roomManager.getRoom(roomCode);
        if (room) {
            if (room->getHostId() != playerName) {
                sendError(ws, "방장만 시작 가능합니다.");
                return;
            }

            if (!room->allReady()) {
                sendError(ws, "모든 플레이어가 레디해야 합니다.");
                return;
            }

            room->setPhase(GamePhase::InGame);
            std::cout << "===== [" << roomCode << "] 게임 시작! =====" << std::endl;

            room->broadcast({
                {"type", "game_started"},
                {"data", {{"roomCode", roomCode}}}
            });
        }
    }

    void handleGetRoomList(WebSocketType* ws, const json& data) {
        json roomList = json::array();
        
        for (const auto& [roomCode, room] : roomManager.getAllRooms()) {
            if (room.getPhase() == GamePhase::Waiting) {
                roomList.push_back({
                    {"roomCode", roomCode},
                    {"playerCount", room.getPlayerCount()}
                });
            }
        }

        ws->send(json({
            {"type", "room_list"},
            {"data", {{"rooms", roomList}}}
        }).dump(), uWS::OpCode::TEXT);
    }

    void handleCheckNickname(WebSocketType* ws, const json& data) {
        std::string playerName = data["playerName"];

        if (roomManager.isNameTakenGlobal(playerName)) {
            sendError(ws, "이미 사용 중인 닉네임입니다.");
            return;
        }

        ws->send(json({
            {"type", "nickname_available"},
            {"data", {{"playerName", playerName}}}
        }).dump(), uWS::OpCode::TEXT);
    }

    void handleLeaveRoom(WebSocketType* ws, const json& data) {
        std::string roomCode = data["roomCode"];
        std::string playerName = data["playerName"];

        Room* room = roomManager.getRoom(roomCode);
        if (room) {
            room->removePlayer(playerName);
            socketMap.erase(ws); // 매핑 제거
            
            std::cout << "[" << roomCode << "] " << playerName << " 님 방에서 퇴장" << std::endl;

            room->broadcast({
                {"type", "player_left"},
                {"data", {{"playerName", playerName}}}
            });

            roomManager.removeRoomIfEmpty(roomCode);
        }
    }

    void onDisconnect(WebSocketType* ws) {
        auto it = socketMap.find(ws);
        if (it != socketMap.end()) {
            std::string roomCode = it->second.roomCode;
            std::string playerName = it->second.playerName;
            
            Room* room = roomManager.getRoom(roomCode);
            if (room) {
                room->removePlayer(playerName);
                std::cout << "[" << roomCode << "] " << playerName << " 님 연결 해제 및 퇴장" << std::endl;

                room->broadcast({
                    {"type", "player_left"}, 
                    {"data", {{"playerName", playerName}}}
                });
                
                roomManager.removeRoomIfEmpty(roomCode);
            }
            socketMap.erase(it);
        }
    }

    void sendError(WebSocketType* ws, const std::string& message) {
        ws->send(json({
            {"type", "error"}, 
            {"data", {{"message", message}}}
        }).dump(), uWS::OpCode::TEXT);
    }
};