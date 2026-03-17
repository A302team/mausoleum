#include "backend/BackendService.h"

#include "backend/BackendConstants.h"
#include "common/logging/Logger.h"
#include "lobby/LobbyConstants.h"
#include "websocket/WebSocketConstants.h"

void BackendService::setRoomManager(RoomManager* inRoomManager) {
    roomManager = inRoomManager;
}

void BackendService::onDomainMessage(WebSocketType* ws, std::string_view type, const json& data) {
    if (type == Backend::Protocol::REQ_REGISTER_DEDICATED) {
        FDedicatedInfo info;
        info.serverId = data.value(std::string(Backend::Protocol::KEY_SERVER_ID), std::string("unknown-dedicated"));
        info.gameHost = data.value(std::string(Backend::Protocol::KEY_GAME_HOST), std::string("127.0.0.1"));
        info.gamePort = data.value(std::string(Backend::Protocol::KEY_GAME_PORT), 0);
        dedicatedConnections[ws] = std::move(info);

        const FDedicatedInfo& registered = dedicatedConnections[ws];
        LOG_INFO("Backend",
                 "[ROLE=DEDICATED] registered serverId=" << registered.serverId
                 << " gameHost=" << registered.gameHost
                 << " gamePort=" << registered.gamePort);

        sendResponse(ws,
                     Backend::Protocol::RES_DEDICATED_REGISTERED,
                     {{std::string(Backend::Protocol::KEY_ROLE), std::string(Backend::Protocol::ROLE_DEDICATED)},
                      {std::string(Backend::Protocol::KEY_SERVER_ID), registered.serverId},
                      {std::string(Backend::Protocol::KEY_GAME_HOST), registered.gameHost},
                      {std::string(Backend::Protocol::KEY_GAME_PORT), registered.gamePort}});
        return;
    }

    if (type == Backend::Protocol::REQ_DEDICATED_READY) {
        auto dedicatedIt = dedicatedConnections.find(ws);
        if (dedicatedIt == dedicatedConnections.end()) {
            LOG_WARN("Backend", "[ROLE=UNKNOWN] dedicated_ready ignored (not a registered dedicated connection)");
            return;
        }

        const std::string roomCode = data.value(std::string(Backend::Protocol::KEY_ROOM_CODE), std::string());
        if (roomCode.empty()) {
            LOG_WARN("Backend", "[ROLE=DEDICATED] dedicated_ready ignored (empty roomCode)");
            return;
        }

        if (pendingStartRooms.find(roomCode) == pendingStartRooms.end()) {
            LOG_WARN("Backend", "[ROLE=DEDICATED] dedicated_ready ignored (room not pending): " << roomCode);
            return;
        }

        if (!roomManager) {
            LOG_ERROR("Backend", "RoomManager not set. Cannot finalize game_started for room " << roomCode);
            return;
        }

        Room* room = roomManager->getRoom(roomCode);
        if (!room) {
            LOG_WARN("Backend", "Room not found while handling dedicated_ready: " << roomCode);
            pendingStartRooms.erase(roomCode);
            return;
        }

        room->setPhase(GamePhase::InGame);
        room->broadcast({{std::string(Lobby::Protocol::KEY_TYPE), std::string(Lobby::Protocol::RES_GAME_STARTED)},
                         {std::string(Lobby::Protocol::KEY_DATA),
                          {{std::string(Lobby::Protocol::KEY_ROOM_CODE), roomCode},
                           {"serverIP", dedicatedIt->second.gameHost},
                           {"serverPort", dedicatedIt->second.gamePort}}}});

        pendingStartRooms.erase(roomCode);
        LOG_INFO("Backend",
                 "[ROLE=DEDICATED] finalized game_started room=" << roomCode
                 << " endpoint=" << dedicatedIt->second.gameHost << ":" << dedicatedIt->second.gamePort);
        return;
    }

    const bool bIsDedicated = dedicatedConnections.find(ws) != dedicatedConnections.end();
    LOG_INFO("Backend",
             "[ROLE=" << (bIsDedicated ? "DEDICATED" : "CLIENT") << "] type=" << std::string(type));
}

void BackendService::onDisconnect(WebSocketType* ws) {
    auto it = dedicatedConnections.find(ws);
    if (it != dedicatedConnections.end()) {
        LOG_INFO("Backend", "[ROLE=DEDICATED] disconnected serverId=" << it->second.serverId);
        dedicatedConnections.erase(it);
    }
}

bool BackendService::requestStartGame(const std::string& roomCode, const std::string& hostPlayerName) {
    WebSocketType* dedicatedWs = chooseDedicated();
    if (!dedicatedWs) {
        LOG_WARN("Backend", "requestStartGame failed. No dedicated registered. room=" << roomCode);
        return false;
    }

    const FDedicatedInfo& dedicated = dedicatedConnections.at(dedicatedWs);
    pendingStartRooms.insert(roomCode);

    sendMessage(dedicatedWs,
                Backend::Protocol::REQ_PREPARE_GAME,
                {{std::string(Backend::Protocol::KEY_ROOM_CODE), roomCode},
                 {std::string(Backend::Protocol::KEY_PLAYER_NAME), hostPlayerName},
                 {std::string(Backend::Protocol::KEY_SERVER_ID), dedicated.serverId},
                 {std::string(Backend::Protocol::KEY_GAME_HOST), dedicated.gameHost},
                 {std::string(Backend::Protocol::KEY_GAME_PORT), dedicated.gamePort}});

    LOG_INFO("Backend",
             "[ROLE=BACKEND] prepare_game sent room=" << roomCode
             << " -> dedicated=" << dedicated.serverId);
    return true;
}

void BackendService::sendResponse(WebSocketType* ws, std::string_view type, const json& data) {
    ws->send(json({{std::string(Lobby::Protocol::KEY_TYPE), std::string(type)},
                   {std::string(Lobby::Protocol::KEY_DATA), data}})
                 .dump(),
             uWS::OpCode::TEXT);
}

void BackendService::sendMessage(WebSocketType* ws, std::string_view type, const json& data) {
    ws->send(json({{std::string(WebSocket::Protocol::KEY_DOMAIN), std::string(Backend::Protocol::BACKEND_DOMAIN)},
                   {std::string(Lobby::Protocol::KEY_TYPE), std::string(type)},
                   {std::string(Lobby::Protocol::KEY_DATA), data}})
                 .dump(),
             uWS::OpCode::TEXT);
}

WebSocketType* BackendService::chooseDedicated() const {
    if (dedicatedConnections.empty()) {
        return nullptr;
    }

    return dedicatedConnections.begin()->first;
}
