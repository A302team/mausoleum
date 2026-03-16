#pragma once
#include <functional>
#include <string>
#include <uwebsockets/App.h>
#include "websocket/WebSocketRouter.h"

class WebSocketServer {
public:
    using DomainHandler = WebSocketRouter::DomainHandler;
    using DisconnectHandler = std::function<void(WebSocketType*)>;

    void setDefaultDomain(std::string domain);
    void registerDomain(std::string domain, DomainHandler handler);
    void setDisconnectHandler(DisconnectHandler handler);

    void run(int port);
    void shutdown();

private:
    WebSocketRouter router_;
    DisconnectHandler onDisconnect_;
    uWS::App* app_ = nullptr;
};
