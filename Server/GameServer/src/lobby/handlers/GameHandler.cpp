#include "GameHandler.h"
#include "common/logging/Logger.h"
#include "../LobbyConstants.h"

using namespace Lobby::Protocol;
using namespace Lobby::Errors;

void GameHandler::sendError(WebSocketType* ws, const std::string& message) {
    ws->send(json({{std::string(KEY_TYPE), RES_ERROR},
                   {std::string(KEY_DATA), {{std::string(KEY_MESSAGE), message}}}})
                 .dump(),
             uWS::OpCode::TEXT);
}

bool GameHandler::extractContext(const json& data, std::string& outRoomCode, std::string& outPlayerName, Room*& outRoom) {
    outRoomCode = data.value(KEY_ROOM_CODE, "");
    outPlayerName = data.value(KEY_PLAYER_NAME, "");
    outRoom = roomManager.getRoom(outRoomCode);
    return outRoom != nullptr;
}

void GameHandler::handleReady(WebSocketType* ws, const json& data) {
    std::string roomCode, playerName;
    Room* room = nullptr;

    if (extractContext(data, roomCode, playerName, room)) {
        Player* player = room->getPlayer(playerName);
        if (player) {
            player->setReady(true);
            LOG_INFO("Lobby", "[" << roomCode << "] " << playerName << " 님 레디!");

            room->broadcast({{std::string(KEY_TYPE), RES_PLAYER_READY},
                             {std::string(KEY_DATA), {{std::string(KEY_PLAYER_NAME), playerName}}}});
        }
    }
}

void GameHandler::handleStartGame(WebSocketType* ws, const json& data) {
    std::string roomCode, playerName;
    Room* room = nullptr;

    if (extractContext(data, roomCode, playerName, room)) {
        if (room->getHostId() != playerName) {
            sendError(ws, std::string(HOST_ONLY_START));
            return;
        }

        if (!room->allReady()) {
            sendError(ws, std::string(ALL_MUST_READY));
            return;
        }

        room->setPhase(GamePhase::InGame);
        LOG_INFO("Lobby", "===== [" << roomCode << "] 게임 시작! =====");

        room->broadcast({{std::string(KEY_TYPE), RES_GAME_STARTED},
                         {std::string(KEY_DATA), {{std::string(KEY_ROOM_CODE), roomCode}}}});
    }
}
