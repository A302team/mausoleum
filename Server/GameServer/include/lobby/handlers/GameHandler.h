#pragma once
#include <nlohmann/json.hpp>
#include <functional>
#include <uwebsockets/App.h>
#include "lobby/domain/RoomManager.h"

using json = nlohmann::json;
using WebSocketType = uWS::WebSocket<false, true, int>;

class GameHandler {
private:
    RoomManager& roomManager;
    std::function<bool(const std::string&, const std::string&)> requestStartGameCallback;

public:
    GameHandler(RoomManager& rm, std::function<bool(const std::string&, const std::string&)> onRequestStartGame)
        : roomManager(rm), requestStartGameCallback(std::move(onRequestStartGame)) {}

    void handleReady(WebSocketType* ws, const json& data);
    void handleStartGame(WebSocketType* ws, const json& data);

private:
    void sendError(WebSocketType* ws, const std::string& message);
    bool extractContext(const json& data, std::string& outRoomCode, std::string& outPlayerName, Room*& outRoom);
};
