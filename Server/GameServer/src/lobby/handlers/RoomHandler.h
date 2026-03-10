#pragma once
#include <nlohmann/json.hpp>
#include <uwebsockets/App.h>
#include "lobby/domain/RoomManager.h"
#include "lobby/LobbyClientManager.h"

using json = nlohmann::json;
using WebSocketType = uWS::WebSocket<false, true, int>;

class RoomHandler {
private:
    RoomManager& roomManager;
    LobbyClientManager& clientManager;

public:
    RoomHandler(RoomManager& rm, LobbyClientManager& cm) 
        : roomManager(rm), clientManager(cm) {}

    void handleCreateRoom(WebSocketType* ws, const json& data);
    void handleJoinRoom(WebSocketType* ws, const json& data);
    void handleGetRoomList(WebSocketType* ws, const json& data);
    void handleCheckNickname(WebSocketType* ws, const json& data);
    void handleLeaveRoom(WebSocketType* ws, const json& data);

private:
    auto createSendFunc(WebSocketType* ws);
    void sendError(WebSocketType* ws, const std::string& message);
    bool checkNameAndSendErrorIfTaken(WebSocketType* ws, const std::string& playerName);
    bool extractContext(const json& data, std::string& outRoomCode, std::string& outPlayerName, Room*& outRoom);
};
