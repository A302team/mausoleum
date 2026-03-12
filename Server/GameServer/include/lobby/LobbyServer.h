#pragma once
#include <uwebsockets/App.h>
#include <iostream>
#include <string_view>
#include "lobby/LobbyService.h"
#include "common/IServer.h"
#include "common/logging/Logger.h"

using WebSocketType = uWS::WebSocket<false, true, int>;

class LobbyServer : public IServer {
private:
    uWS::App* app = nullptr; // 포인터로 변경할 필요성에 대해서: uWS 종료 관리 시 유용
    LobbyService service;
public:
    LobbyServer();

    // IServer Interface Override
    void run(int port) override;
    void shutdown() override;

protected:
    const char* tag() const override { return "Lobby"; }
    bool initSocket(int port) override { return true; }
    void runLoop() override {}

private:
    void onMessage(WebSocketType *ws, std::string_view msg);
    void onDisconnect(WebSocketType *ws);
};
