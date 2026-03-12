#pragma once
#include <nlohmann/json.hpp>
#include <functional>
#include <unordered_map>
#include <string>
#include <iostream>
#include <memory>
#include <uwebsockets/App.h>
#include "lobby/domain/RoomManager.h"
#include "lobby/LobbyClientManager.h"
#include "common/logging/Logger.h"

using json = nlohmann::json;
using WebSocketType = uWS::WebSocket<false, true, int>;

class LobbyPacketRouter {
private:
    using MessageHandler = std::function<void(WebSocketType*, const json&)>;
    std::unordered_map<std::string, MessageHandler> handlers;
    
    RoomManager& roomManager;
    LobbyClientManager& clientManager;

    class Impl;
    std::unique_ptr<Impl> pImpl;

public:
    LobbyPacketRouter(RoomManager& rm, LobbyClientManager& cm);
    ~LobbyPacketRouter();

    void dispatch(WebSocketType* ws, std::string_view msg);

private:
    void registerHandlers();
};
