#pragma once
#include <nlohmann/json.hpp>
#include <functional>
#include <unordered_map>
#include <string>
#include <iostream>
#include <uwebsockets/App.h>
#include "domain/RoomManager.h"
#include "LobbyClientManager.h"
#include "common/logging/Logger.h"

using json = nlohmann::json;
using WebSocketType = uWS::WebSocket<false, true, int>;

class LobbyPacketRouter {
private:
    using MessageHandler = std::function<void(WebSocketType*, const json&)>;
    std::unordered_map<std::string, MessageHandler> handlers;
    
    RoomManager& roomManager;
    LobbyClientManager& clientManager;

public:
    LobbyPacketRouter(RoomManager& rm, LobbyClientManager& cm);

    void dispatch(WebSocketType* ws, std::string_view msg);

private:
    void registerHandlers();
    auto createSendFunc(WebSocketType* ws);

    // 공통 로직 추출을 위한 헬퍼 함수
    void sendError(WebSocketType* ws, const std::string& message);
    bool checkNameAndSendErrorIfTaken(WebSocketType* ws, const std::string& playerName);
    bool extractContext(const json& data, std::string& outRoomCode, std::string& outPlayerName, Room*& outRoom);

    // 핸들러
    void handleCreateRoom(WebSocketType* ws, const json& data);
    void handleJoinRoom(WebSocketType* ws, const json& data);
    void handleReady(WebSocketType* ws, const json& data);
    void handleStartGame(WebSocketType* ws, const json& data);
    void handleGetRoomList(WebSocketType* ws, const json& data);
    void handleCheckNickname(WebSocketType* ws, const json& data);
    void handleLeaveRoom(WebSocketType* ws, const json& data);
    void handleChatMessage(WebSocketType* ws, const json& data);
};
