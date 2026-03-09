#include "LobbyPacketRouter.h"
#include "handlers/RoomHandler.h"
#include "handlers/GameHandler.h"
#include "handlers/ChatHandler.h"
#include "LobbyConstants.h"

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
    try
    {
        json received = json::parse(msg);
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
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Lobby", "JSON 파싱 오류: " << e.what());
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

