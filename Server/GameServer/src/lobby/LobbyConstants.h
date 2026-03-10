#pragma once
#include <string_view>

namespace Lobby::Protocol {

    // JSON Keys
    constexpr std::string_view KEY_TYPE = "type";
    constexpr std::string_view KEY_DATA = "data";
    constexpr std::string_view KEY_MESSAGE = "message";
    constexpr std::string_view KEY_ROOM_CODE = "roomCode";
    constexpr std::string_view KEY_PLAYER_NAME = "playerName";
    constexpr std::string_view KEY_IS_HOST = "isHost";
    constexpr std::string_view KEY_ROOMS = "rooms";
    constexpr std::string_view KEY_PLAYER_COUNT = "playerCount";

    // Request Types
    constexpr std::string_view REQ_CREATE_ROOM = "create_room";
    constexpr std::string_view REQ_JOIN_ROOM = "join_room";
    constexpr std::string_view REQ_READY = "ready";
    constexpr std::string_view REQ_START_GAME = "start_game";
    constexpr std::string_view REQ_GET_ROOM_LIST = "get_room_list";
    constexpr std::string_view REQ_CHECK_NICKNAME = "check_nickname";
    constexpr std::string_view REQ_LEAVE_ROOM = "leave_room";
    constexpr std::string_view REQ_CHAT_MESSAGE = "chat_message";

    // Response Types
    constexpr std::string_view RES_ERROR = "error";
    constexpr std::string_view RES_ROOM_CREATED = "room_created";
    constexpr std::string_view RES_ROOM_JOINED = "room_joined";
    constexpr std::string_view RES_PLAYER_ENTERED = "player_entered";
    constexpr std::string_view RES_PLAYER_READY = "player_ready";
    constexpr std::string_view RES_GAME_STARTED = "game_started";
    constexpr std::string_view RES_ROOM_LIST = "room_list";
    constexpr std::string_view RES_NICKNAME_AVAILABLE = "nickname_available";
    constexpr std::string_view RES_PLAYER_LEFT = "player_left";
    constexpr std::string_view RES_CHAT_MESSAGE = "chat_message";

} // namespace Lobby::Protocol

namespace Lobby::Errors {
    constexpr std::string_view NAME_TAKEN = "이미 사용 중인 닉네임입니다.";
    constexpr std::string_view ROOM_NOT_FOUND = "존재하지 않는 코드입니다.";
    constexpr std::string_view HOST_ONLY_START = "방장만 시작 가능합니다.";
    constexpr std::string_view ALL_MUST_READY = "모든 플레이어가 레디해야 합니다.";
} // namespace Lobby::Errors
