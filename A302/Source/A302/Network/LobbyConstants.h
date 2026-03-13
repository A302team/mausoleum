#pragma once

#include "CoreMinimal.h"

/**
 * Lobby Protocol Constants for Unreal Engine Client
 * Keeps the Client synchronized with the C++ LobbyServer definitions.
 */
namespace LobbyProtocol
{
    // JSON Keys
    const FString KeyType = TEXT("type");
    const FString KeyData = TEXT("data");
    const FString KeyMessage = TEXT("message");
    const FString KeyRoomCode = TEXT("roomCode");
    const FString KeyPlayerName = TEXT("playerName");
    const FString KeyIsHost = TEXT("isHost");
    const FString KeyGameServerAddress = TEXT("gameServerAddress");
    const FString KeyRooms = TEXT("rooms");
    const FString KeyPlayerCount = TEXT("playerCount");

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
    const FString ResError = TEXT("error");
    const FString ResRoomCreated = TEXT("room_created");
    const FString ResRoomJoined = TEXT("room_joined");
    const FString ResPlayerEntered = TEXT("player_entered");
    const FString ResPlayerReady = TEXT("player_ready");
    const FString ResGameStarted = TEXT("game_started");
    const FString ResRoomList = TEXT("room_list");
    const FString ResNicknameAvailable = TEXT("nickname_available");
    const FString ResPlayerLeft = TEXT("player_left");
    const FString ResChatMessage = TEXT("chat_message");
}
