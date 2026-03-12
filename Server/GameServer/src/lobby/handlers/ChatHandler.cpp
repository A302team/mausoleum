#include "lobby/handlers/ChatHandler.h"
#include "common/logging/Logger.h"
#include "lobby/LobbyConstants.h"

using namespace Lobby::Protocol;

bool ChatHandler::extractContext(const json& data, std::string& outRoomCode, std::string& outPlayerName, Room*& outRoom) {
    outRoomCode = data.value(KEY_ROOM_CODE, "");
    outPlayerName = data.value(KEY_PLAYER_NAME, "");
    outRoom = roomManager.getRoom(outRoomCode);
    return outRoom != nullptr;
}

void ChatHandler::handleChatMessage(WebSocketType* ws, const json& data) {
    std::string roomCode, playerName;
    Room* room = nullptr;
    std::string message = data.value(KEY_MESSAGE, "");

    if (extractContext(data, roomCode, playerName, room)) {
        LOG_INFO("Lobby", "[" << roomCode << "] " << playerName << ": " << message);

        room->broadcast({{std::string(KEY_TYPE), RES_CHAT_MESSAGE}, {std::string(KEY_DATA), {{std::string(KEY_PLAYER_NAME), playerName}, {std::string(KEY_MESSAGE), message}}}});
    }
}
