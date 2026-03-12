#include "websocket/WebSocketServer.h"
#include "common/logging/Logger.h"

void WebSocketServer::setDefaultDomain(std::string domain) {
    router_.setDefaultDomain(std::move(domain));
}

void WebSocketServer::registerDomain(std::string domain, DomainHandler handler) {
    router_.registerDomain(std::move(domain), std::move(handler));
}

void WebSocketServer::setDisconnectHandler(DisconnectHandler handler) {
    onDisconnect_ = std::move(handler);
}

void WebSocketServer::run(int port) {
    app_ = new uWS::App();
    app_->ws<int>("/*", {
        .open = [](auto *)
        {
            LOG_INFO("WebSocket", "클라이언트 접속!");
        },
        .message = [this](auto *ws, std::string_view msg, uWS::OpCode)
        {
            router_.dispatch(ws, msg);
        },
        .close = [this](auto *ws, int, std::string_view)
        {
            LOG_INFO("WebSocket", "클라이언트 연결 종료!");
            if (onDisconnect_) {
                onDisconnect_(ws);
            }
        }
    })
    .listen(port, [port](auto *token)
    {
        if (token)
        {
            LOG_INFO("WebSocket", "서버 시작! 포트: " << port);
        }
        else
        {
            LOG_ERROR("WebSocket", "서버 시작 실패! 포트: " << port << "가 사용 중일 수 있습니다.");
        }
    })
    .run();
}

void WebSocketServer::shutdown() {
    // uWS의 명시적 stop API가 없어 현재는 no-op
}
