#include "lobby/LobbyPacketRouter.h"
#include "lobby/handlers/RoomHandler.h"
#include "lobby/handlers/GameHandler.h"
#include "lobby/handlers/ChatHandler.h"
#include "lobby/LobbyConstants.h"

using namespace Lobby::Protocol;

class LobbyPacketRouter::Impl {
public:
    RoomHandler roomHandler;
    GameHandler gameHandler;
    ChatHandler chatHandler;

    Impl(RoomManager& rm, LobbyClientManager& cm)
        : roomHandler(rm, cm), gameHandler(rm), chatHandler(rm) {}
};

LobbyPacketRouter::LobbyPacketRouter(RoomManager& rm, LobbyClientManager& cm)
    : roomManager(rm), clientManager(cm), pImpl(std::make_unique<Impl>(rm, cm))
{
    registerHandlers();
}

LobbyPacketRouter::~LobbyPacketRouter() = default;

void LobbyPacketRouter::dispatch(WebSocketType* ws, std::string_view msg)
{
    json received = json::parse(msg, nullptr, false);
    if (received.is_discarded() || !received.is_object()) {
        LOG_WARN("Lobby", "JSON 파싱 오류");
        sendError(ws, Lobby::Errors::INVALID_JSON);
        return;
    }

    if (!received.contains(KEY_TYPE) || !received[KEY_TYPE].is_string()
        || !received.contains(KEY_DATA) || !received[KEY_DATA].is_object()) {
        LOG_WARN("Lobby", "잘못된 메시지 형식");
        sendError(ws, Lobby::Errors::INVALID_MESSAGE);
        return;
    }

    std::string type = received[KEY_TYPE];
    json data = received[KEY_DATA];

    auto it = handlers.find(type);
    if (it != handlers.end())
    {
        it->second(ws, data);
    }
    else
    {
        LOG_WARN("Lobby", "알 수 없는 메시지 타입: " << type);
        sendError(ws, Lobby::Errors::UNKNOWN_TYPE);
    }
}

void LobbyPacketRouter::registerHandlers()
{
    handlers[std::string(REQ_CREATE_ROOM)] = [this](auto* ws, const json& data) { pImpl->roomHandler.handleCreateRoom(ws, data); };
    handlers[std::string(REQ_JOIN_ROOM)] = [this](auto* ws, const json& data) { pImpl->roomHandler.handleJoinRoom(ws, data); };
    handlers[std::string(REQ_GET_ROOM_LIST)] = [this](auto* ws, const json& data) { pImpl->roomHandler.handleGetRoomList(ws, data); };
    handlers[std::string(REQ_CHECK_NICKNAME)] = [this](auto* ws, const json& data) { pImpl->roomHandler.handleCheckNickname(ws, data); };
    handlers[std::string(REQ_LEAVE_ROOM)] = [this](auto* ws, const json& data) { pImpl->roomHandler.handleLeaveRoom(ws, data); };

    handlers[std::string(REQ_READY)] = [this](auto* ws, const json& data) { pImpl->gameHandler.handleReady(ws, data); };
    handlers[std::string(REQ_START_GAME)] = [this](auto* ws, const json& data) { pImpl->gameHandler.handleStartGame(ws, data); };

    handlers[std::string(REQ_CHAT_MESSAGE)] = [this](auto* ws, const json& data) { pImpl->chatHandler.handleChatMessage(ws, data); };
}

void LobbyPacketRouter::sendError(WebSocketType* ws, std::string_view message) {
    ws->send(json({{std::string(KEY_TYPE), RES_ERROR},
                   {std::string(KEY_DATA), {{std::string(KEY_MESSAGE), std::string(message)}}}})
                 .dump(),
             uWS::OpCode::TEXT);
}
