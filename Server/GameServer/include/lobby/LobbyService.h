#pragma once
#include <string_view>
#include <uwebsockets/App.h>
#include "lobby/domain/RoomManager.h"
#include "lobby/LobbyClientManager.h"
#include "lobby/LobbyPacketRouter.h"

using WebSocketType = uWS::WebSocket<false, true, int>;

class LobbyService {
public:
    LobbyService();

    void onDomainMessage(WebSocketType* ws, std::string_view type, const json& data);
    void onDisconnect(WebSocketType* ws);

private:
    RoomManager roomManager;
    LobbyClientManager clientManager;
    LobbyPacketRouter router;
};
