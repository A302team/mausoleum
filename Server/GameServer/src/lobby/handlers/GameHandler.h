#pragma once
#include <nlohmann/json.hpp>
#include <uwebsockets/App.h>
#include "lobby/domain/RoomManager.h"

using json = nlohmann::json;
using WebSocketType = uWS::WebSocket<false, true, int>;

class GameHandler {
private:
    RoomManager& roomManager;

public:
    explicit GameHandler(RoomManager& rm) : roomManager(rm) {}

    void handleReady(WebSocketType* ws, const json& data);
    void handleStartGame(WebSocketType* ws, const json& data);

private:
    void sendError(WebSocketType* ws, const std::string& message);
    bool extractContext(const json& data, std::string& outRoomCode, std::string& outPlayerName, Room*& outRoom);
};
