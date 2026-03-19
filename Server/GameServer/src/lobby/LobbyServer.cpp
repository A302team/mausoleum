#include "lobby/LobbyServer.h"
#include "backend/BackendConstants.h"

LobbyServer::LobbyServer()
    : service([this](const std::string& roomCode, const std::string& hostPlayerName) {
          return backendService.requestStartGame(roomCode, hostPlayerName);
      })
{
    backendService.setRoomManager(&service.getRoomManager());
}

void LobbyServer::run(int port)
{
    running = true;

    wsServer.setDefaultDomain(std::string(Lobby::Protocol::LOBBY_DOMAIN));
    wsServer.registerDomain(std::string(Lobby::Protocol::LOBBY_DOMAIN),
        [this](auto* ws, std::string_view type, const json& data) {
            service.onDomainMessage(ws, type, data);
        });
    wsServer.registerDomain(std::string(Backend::Protocol::BACKEND_DOMAIN),
        [this](auto* ws, std::string_view type, const json& data) {
            backendService.onDomainMessage(ws, type, data);
        });
    wsServer.setDisconnectHandler([this](auto* ws) {
        service.onDisconnect(ws);
        backendService.onDisconnect(ws);
    });

    wsServer.run(port);
}

void LobbyServer::shutdown()
{
    running = false;
}
