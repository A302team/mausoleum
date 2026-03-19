#pragma once
#include <iostream>
#include <string_view>
#include "backend/BackendService.h"
#include "lobby/LobbyService.h"
#include "websocket/WebSocketServer.h"
#include "common/IServer.h"
#include "common/logging/Logger.h"

class LobbyServer : public IServer {
private:
    WebSocketServer wsServer;
    LobbyService service;
    BackendService backendService;
public:
    LobbyServer();

    // IServer Interface Override
    void run(int port) override;
    void shutdown() override;

protected:
    const char* tag() const override { return "Lobby"; }
    bool initSocket(int port) override { return true; }
    void runLoop() override {}
};
