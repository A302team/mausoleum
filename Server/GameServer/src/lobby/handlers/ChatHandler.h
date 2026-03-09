#pragma once
#include <nlohmann/json.hpp>
#include <uwebsockets/App.h>
#include "lobby/domain/RoomManager.h"

using json = nlohmann::json;
using WebSocketType = uWS::WebSocket<false, true, int>;

class ChatHandler {
private:
    RoomManager& roomManager;

public:
    explicit ChatHandler(RoomManager& rm) : roomManager(rm) {}

    void handleChatMessage(WebSocketType* ws, const json& data);

private:
    bool extractContext(const json& data, std::string& outRoomCode, std::string& outPlayerName, Room*& outRoom);
};
