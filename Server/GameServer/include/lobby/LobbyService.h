#pragma once
#include <functional>
#include <string>
#include <string_view>
#include <uwebsockets/App.h>
#include "lobby/domain/RoomManager.h"
#include "lobby/LobbyClientManager.h"
#include "lobby/LobbyPacketRouter.h"

using WebSocketType = uWS::WebSocket<false, true, int>;

class LobbyService {
public:
    explicit LobbyService(std::function<bool(const std::string&, const std::string&)> onRequestStartGame);

    void onDomainMessage(WebSocketType* ws, std::string_view type, const json& data);
    void onDisconnect(WebSocketType* ws);
    RoomManager& getRoomManager() { return roomManager; }

private:
    RoomManager roomManager;
    LobbyClientManager clientManager;
    LobbyPacketRouter router;
};
