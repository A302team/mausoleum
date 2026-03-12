#include "lobby/LobbyServer.h"

LobbyServer::LobbyServer() : router(roomManager, clientManager)
{
}

void LobbyServer::run(int port)
{
    running = true;
    app = new uWS::App();
    app->ws<int>("/*", {
        .open = [](auto *ws)
        {
            LOG_INFO("Lobby", "클라이언트 접속!");
        },
        .message = [this](auto *ws, std::string_view msg, uWS::OpCode opCode)
        {
            this->onMessage(ws, msg);
        },
        .close = [this](auto *ws, int, std::string_view)
        {
            this->onDisconnect(ws);
        }
    })
    .listen(port, [port](auto *token)
    {
        if (token) 
        {
            LOG_INFO("Lobby", "서버 시작! 포트: " << port);
        } 
        else 
        {
            LOG_ERROR("Lobby", "서버 시작 실패! 포트: " << port << "가 사용 중일 수 있습니다.");
        }
    })
    .run();
}

void LobbyServer::shutdown()
{
    running = false;
}

void LobbyServer::onMessage(WebSocketType *ws, std::string_view msg)
{
    router.dispatch(ws, msg);
}

void LobbyServer::onDisconnect(WebSocketType *ws)
{
    auto* info = clientManager.getClientInfo(ws);
    if (info)
    {
        std::string roomCode = info->roomCode;
        std::string playerName = info->playerName;

        Room* room = roomManager.getRoom(roomCode);
        if (room)
        {
            room->removePlayer(playerName);
            LOG_INFO("Lobby", "[" << roomCode << "] " << playerName << " 님 연결 해제 및 퇴장");

            room->broadcast({{"type", "player_left"},
                             {"data", {{"playerName", playerName}}}});

            roomManager.removeRoomIfEmpty(roomCode);
        }
        clientManager.removeClient(ws);
    }
}
