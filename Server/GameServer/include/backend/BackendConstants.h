#pragma once
#include <string_view>

namespace Backend::Protocol {
    constexpr std::string_view BACKEND_DOMAIN = "backend";

    constexpr std::string_view KEY_ROLE = "role";
    constexpr std::string_view KEY_SERVER_ID = "serverId";
    constexpr std::string_view KEY_ROOM_CODE = "roomCode";
    constexpr std::string_view KEY_PLAYER_NAME = "playerName";
    constexpr std::string_view KEY_GAME_HOST = "gameHost";
    constexpr std::string_view KEY_GAME_PORT = "gamePort";

    constexpr std::string_view REQ_REGISTER_DEDICATED = "register_dedicated";
    constexpr std::string_view REQ_PREPARE_GAME = "prepare_game";
    constexpr std::string_view REQ_DEDICATED_READY = "dedicated_ready";
    constexpr std::string_view RES_DEDICATED_REGISTERED = "dedicated_registered";

    constexpr std::string_view ROLE_DEDICATED = "dedicated";
    constexpr std::string_view ROLE_CLIENT = "client";
}
