#pragma once

#include "CoreMinimal.h"

/**
 * Lobby Protocol Constants for Unreal Engine Client
 * Keeps the Client synchronized with the C++ LobbyServer definitions.
 */
namespace LobbyProtocol
{
    const FString KeyDomain = TEXT("domain");
    // JSON Keys
    const FString KeyType = TEXT("type");
    const FString KeyData = TEXT("data");
    const FString KeyMessage = TEXT("message");
    const FString KeyRoomCode = TEXT("roomCode");
    const FString KeyPlayerName = TEXT("playerName");
    const FString KeyIsHost = TEXT("isHost");
    const FString KeyGameServerAddress = TEXT("gameServerAddress");
    const FString KeyServerIP = TEXT("serverIP");
    const FString KeyServerPort = TEXT("serverPort");
    const FString KeyRooms = TEXT("rooms");
    const FString KeyPlayerCount = TEXT("playerCount");
    const FString KeyItemType = TEXT("itemType");

    // Request Types
    const FString ReqCreateRoom = TEXT("create_room");
    const FString ReqJoinRoom = TEXT("join_room");
    const FString ReqReady = TEXT("ready");
    const FString ReqStartGame = TEXT("start_game");
    const FString ReqGetRoomList = TEXT("get_room_list");
    const FString ReqCheckNickname = TEXT("check_nickname");
    const FString ReqLeaveRoom = TEXT("leave_room");
    const FString ReqChatMessage = TEXT("chat_message");

    // Response Types
    const FString ResRoomScopedMessage = TEXT("room_scoped_message");
    const FString ResError = TEXT("error");
    const FString ResRoomCreated = TEXT("room_created");
    const FString ResRoomJoined = TEXT("room_joined");
    const FString ResPlayerEntered = TEXT("player_entered");
    const FString ResPlayerReady = TEXT("player_ready");
    const FString ResGameStarted = TEXT("game_started");
    const FString ResRoomList = TEXT("room_list");
    const FString ResNicknameAvailable = TEXT("nickname_available");
    const FString ResPlayerLeft = TEXT("player_left");
    const FString ResHostChanged = TEXT("host_changed");
    const FString ResChatMessage = TEXT("chat_message");
}

namespace BackendProtocol
{
    const FString Domain = TEXT("backend");

    const FString KeyRole = TEXT("role");
    const FString KeyServerId = TEXT("serverId");
    const FString KeyRoomCode = TEXT("roomCode");
    const FString KeyPlayerName = TEXT("playerName");
    const FString KeyGameHost = TEXT("gameHost");
    const FString KeyGamePort = TEXT("gamePort");

    const FString ReqRegisterDedicated = TEXT("register_dedicated");
    const FString ReqPrepareGame = TEXT("prepare_game");
    const FString ReqDedicatedReady = TEXT("dedicated_ready");
    const FString ReqFinishGame = TEXT("finish_game");
    const FString ResDedicatedRegistered = TEXT("dedicated_registered");

    const FString RoleDedicated = TEXT("dedicated");
    const FString RoleClient = TEXT("client");
}
