#include "lobby/LobbyPacketRouter.h"
#include "lobby/handlers/RoomHandler.h"
#include "lobby/handlers/GameHandler.h"
#include "lobby/handlers/ChatHandler.h"
#include "lobby/LobbyConstants.h"
#include <utility>

using namespace Lobby::Protocol;

class LobbyPacketRouter::Impl {
public:
    RoomHandler roomHandler;
    GameHandler gameHandler;
    ChatHandler chatHandler;

    Impl(
        RoomManager& rm,
        LobbyClientManager& cm,
        std::function<bool(const std::string&, const std::string&)> onRequestStartGame
    )
        : roomHandler(rm, cm), gameHandler(rm, std::move(onRequestStartGame)), chatHandler(rm) {}
};

LobbyPacketRouter::LobbyPacketRouter(
    RoomManager& rm,
    LobbyClientManager& cm,
    std::function<bool(const std::string&, const std::string&)> onRequestStartGame
)
    : roomManager(rm), clientManager(cm), pImpl(std::make_unique<Impl>(rm, cm, std::move(onRequestStartGame)))
{
    registerHandlers();
}

LobbyPacketRouter::~LobbyPacketRouter() = default;

void LobbyPacketRouter::dispatch(WebSocketType* ws, std::string_view type, const json& data)
{
    auto it = handlers.find(std::string(type));
    if (it != handlers.end())
    {
        it->second(ws, data);
    }
    else
    {
        LOG_WARN("Lobby", "알 수 없는 메시지 타입: " << type);
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
