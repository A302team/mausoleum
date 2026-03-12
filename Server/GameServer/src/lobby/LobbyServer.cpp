#include "lobby/LobbyServer.h"

LobbyServer::LobbyServer() = default;

void LobbyServer::run(int port)
{
    running = true;

    wsServer.setDefaultDomain(std::string(Lobby::Protocol::LOBBY_DOMAIN));
    wsServer.registerDomain(std::string(Lobby::Protocol::LOBBY_DOMAIN),
        [this](auto* ws, std::string_view type, const json& data) {
            service.onDomainMessage(ws, type, data);
        });
    wsServer.setDisconnectHandler([this](auto* ws) { service.onDisconnect(ws); });

    wsServer.run(port);
}

void LobbyServer::shutdown()
{
    running = false;
}
