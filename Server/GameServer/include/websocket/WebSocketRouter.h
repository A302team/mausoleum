#pragma once
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <uwebsockets/App.h>
#include <nlohmann/json.hpp>
#include "websocket/WebSocketConstants.h"

using json = nlohmann::json;
using WebSocketType = uWS::WebSocket<false, true, int>;

class WebSocketRouter {
public:
    using DomainHandler = std::function<void(WebSocketType*, std::string_view type, const json& data)>;

    void setDefaultDomain(std::string domain);
    void registerDomain(std::string domain, DomainHandler handler);
    void dispatch(WebSocketType* ws, std::string_view msg);

private:
    std::unordered_map<std::string, DomainHandler> handlers_;
    std::optional<std::string> defaultDomain_;

    void sendError(WebSocketType* ws, std::string_view message);
};
