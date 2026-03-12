#pragma once
#include <string_view>

namespace WebSocket::Protocol {
    constexpr std::string_view KEY_DOMAIN = "domain";
    constexpr std::string_view KEY_TYPE = "type";
    constexpr std::string_view KEY_DATA = "data";
    constexpr std::string_view KEY_MESSAGE = "message";

    constexpr std::string_view RES_ERROR = "error";
}

namespace WebSocket::Errors {
    constexpr std::string_view INVALID_JSON = "잘못된 JSON 형식입니다.";
    constexpr std::string_view INVALID_MESSAGE = "잘못된 메시지 형식입니다.";
    constexpr std::string_view UNKNOWN_DOMAIN = "알 수 없는 도메인입니다.";
}
