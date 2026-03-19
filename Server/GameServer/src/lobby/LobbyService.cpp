#include "lobby/LobbyService.h"
#include "common/logging/Logger.h"
#include "lobby/LobbyConstants.h"
#include <utility>

LobbyService::LobbyService(std::function<bool(const std::string&, const std::string&)> onRequestStartGame)
    : router(roomManager, clientManager, std::move(onRequestStartGame)) {}

void LobbyService::onDomainMessage(WebSocketType* ws, std::string_view type, const json& data) {
    LOG_INFO("Lobby", "[ROLE=CLIENT] type=" << std::string(type));
    router.dispatch(ws, type, data);
}

void LobbyService::onDisconnect(WebSocketType* ws) {
    auto* info = clientManager.getClientInfo(ws);
    if (info) {
        std::string roomCode = info->roomCode;
        std::string playerName = info->playerName;

        Room* room = roomManager.getRoom(roomCode);
        if (room) {
            room->removePlayer(playerName);
            LOG_INFO("Lobby", "[" << roomCode << "] " << playerName << " 님 연결 해제 및 퇴장");

            room->broadcast({{std::string(Lobby::Protocol::KEY_TYPE), std::string(Lobby::Protocol::RES_PLAYER_LEFT)},
                             {std::string(Lobby::Protocol::KEY_DATA),
                              {{std::string(Lobby::Protocol::KEY_PLAYER_NAME), playerName}}}});

            roomManager.removeRoomIfEmpty(roomCode);
        }
        clientManager.removeClient(ws);
    }
}
