#pragma once

#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <uwebsockets/App.h>
#include <nlohmann/json.hpp>
#include "lobby/domain/RoomManager.h"

using json = nlohmann::json;
using WebSocketType = uWS::WebSocket<false, true, int>;

struct FDedicatedInfo {
    std::string serverId;
    std::string gameHost;
    int gamePort = 0;
};

class BackendService {
public:
    void setRoomManager(RoomManager* inRoomManager);
    void onDomainMessage(WebSocketType* ws, std::string_view type, const json& data);
    void onDisconnect(WebSocketType* ws);
    bool requestStartGame(const std::string& roomCode, const std::string& hostPlayerName);

private:
    void sendResponse(WebSocketType* ws, std::string_view type, const json& data);
    void sendMessage(WebSocketType* ws, std::string_view type, const json& data);
    WebSocketType* chooseDedicated() const;

    RoomManager* roomManager = nullptr;
    std::unordered_map<WebSocketType*, FDedicatedInfo> dedicatedConnections;
    std::unordered_set<std::string> pendingStartRooms;
};
