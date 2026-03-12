#include "websocket/WebSocketRouter.h"

void WebSocketRouter::setDefaultDomain(std::string domain) {
    defaultDomain_ = std::move(domain);
}

void WebSocketRouter::registerDomain(std::string domain, DomainHandler handler) {
    handlers_[std::move(domain)] = std::move(handler);
}

void WebSocketRouter::dispatch(WebSocketType* ws, std::string_view msg) {
    json received = json::parse(msg, nullptr, false);
    if (received.is_discarded() || !received.is_object()) {
        sendError(ws, WebSocket::Errors::INVALID_JSON);
        return;
    }

    std::string domain;
    if (received.contains(WebSocket::Protocol::KEY_DOMAIN) && received[WebSocket::Protocol::KEY_DOMAIN].is_string()) {
        domain = received[WebSocket::Protocol::KEY_DOMAIN].get<std::string>();
    } else if (defaultDomain_) {
        domain = *defaultDomain_;
    } else {
        sendError(ws, WebSocket::Errors::INVALID_MESSAGE);
        return;
    }

    if (!received.contains(WebSocket::Protocol::KEY_TYPE) || !received[WebSocket::Protocol::KEY_TYPE].is_string()
        || !received.contains(WebSocket::Protocol::KEY_DATA) || !received[WebSocket::Protocol::KEY_DATA].is_object()) {
        sendError(ws, WebSocket::Errors::INVALID_MESSAGE);
        return;
    }

    std::string type = received[WebSocket::Protocol::KEY_TYPE].get<std::string>();
    json data = received[WebSocket::Protocol::KEY_DATA];

    auto it = handlers_.find(domain);
    if (it == handlers_.end()) {
        sendError(ws, WebSocket::Errors::UNKNOWN_DOMAIN);
        return;
    }

    it->second(ws, type, data);
}

void WebSocketRouter::sendError(WebSocketType* ws, std::string_view message) {
    ws->send(json({{std::string(WebSocket::Protocol::KEY_TYPE), std::string(WebSocket::Protocol::RES_ERROR)},
                   {std::string(WebSocket::Protocol::KEY_DATA),
                    {{std::string(WebSocket::Protocol::KEY_MESSAGE), std::string(message)}}}})
                 .dump(),
             uWS::OpCode::TEXT);
}
