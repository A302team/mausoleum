// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/A302GameInstance.h"
#include "Network/WebSocketManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Kismet/GameplayStatics.h"

void UA302GameInstance::Init()
{
    Super::Init();

    WebSocketManager = NewObject<UWebSocketManager>(this, TEXT("WebSocketManager"));
    WebSocketManager->OnMessageReceived.AddDynamic(this, &UA302GameInstance::OnMessageReceived);

    ConnectToServer(TEXT("ws://localhost:9001"));

    UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] Init - WebSocket 연결 시도"));
}

void UA302GameInstance::ConnectToServer(const FString &URL)
{
    if (WebSocketManager)
    {
        WebSocketManager->Connect(URL);
    }
}

void UA302GameInstance::SendToServer(const FString &Message)
{
    if (WebSocketManager)
    {
        WebSocketManager->SendMessage(Message);
    }
}

void UA302GameInstance::OnMessageReceived(const FString &Message)
{
    UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 수신: %s"), *Message);

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject))
        return;

    FString Type = JsonObject->GetStringField(TEXT("type"));
    TSharedPtr<FJsonObject> Data = JsonObject->GetObjectField(TEXT("data"));

    if (Type == TEXT("room_created"))
    {
        CurrentRoomCode = Data->GetStringField(TEXT("roomCode"));
        MyPlayerName = Data->GetStringField(TEXT("playerName"));
        bIsHost = true;
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 방 생성: %s"), *CurrentRoomCode);
        OnRoomCreated.Broadcast(CurrentRoomCode);
    }
    else if (Type == TEXT("room_joined"))
    {
        CurrentRoomCode = Data->GetStringField(TEXT("roomCode"));
        MyPlayerName = Data->GetStringField(TEXT("playerName"));
        bIsHost = Data->GetBoolField(TEXT("isHost"));
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 방 입장: %s"), *CurrentRoomCode);
        OnRoomJoined.Broadcast();
    }
    else if (Type == TEXT("player_entered"))
    {
        FString EnteredName = Data->GetStringField(TEXT("playerName"));
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 플레이어 입장: %s"), *EnteredName);
        OnPlayerEntered.Broadcast(EnteredName);
    }
    else if (Type == TEXT("room_list"))
    {
        TArray<FRoomInfo> RoomList;
        const TArray<TSharedPtr<FJsonValue>> *Rooms;
        if (Data->TryGetArrayField(TEXT("rooms"), Rooms))
        {
            for (auto &Room : *Rooms)
            {
                FRoomInfo Info;
                Info.RoomCode = Room->AsObject()->GetStringField(TEXT("roomCode"));
                Info.PlayerCount = Room->AsObject()->GetIntegerField(TEXT("playerCount"));
                RoomList.Add(Info);
            }
        }
        OnRoomListReceived.Broadcast(RoomList);
    }
    else if (Type == TEXT("player_ready"))
    {
        FString ReadyName = Data->GetStringField(TEXT("playerName"));
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 플레이어 레디: %s"), *ReadyName);
        OnPlayerReady.Broadcast(ReadyName);
    }
    else if (Type == TEXT("player_left"))
    {
        FString LeftName = Data->GetStringField(TEXT("playerName"));
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 플레이어 퇴장: %s"), *LeftName);
        OnPlayerLeft.Broadcast(LeftName);
    }
    else if (Type == TEXT("game_started"))
    {
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 게임 시작!"));
        OnGameStarted.Broadcast();
        UGameplayStatics::OpenLevel(this, TEXT("/Game/PersonalWorkSpace/sikk806/MyTestLevel"));
    }
    else if (Type == TEXT("nickname_available"))
    {
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 닉네임 사용 가능"));
        OnNicknameAvailable.Broadcast();
    }
    else if (Type == TEXT("chat_message"))
    {
        FString PlayerName = Data->GetStringField(TEXT("playerName"));
        FString ChatMessage = Data->GetStringField(TEXT("message"));
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 채팅: %s: %s"), *PlayerName, *ChatMessage);
        OnChatMessageReceived.Broadcast(PlayerName, ChatMessage);
    }
    else if (Type == TEXT("item_received"))
    {
        FString PlayerName = Data->GetStringField(TEXT("playerName"));
        FString ItemType = Data->GetStringField(TEXT("itemType"));
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 아이템 수신: %s → %s"), *PlayerName, *ItemType);
        OnItemReceived.Broadcast(PlayerName, ItemType);
    }
    else if (Type == TEXT("error"))
    {
        FString ErrorMsg = Data->GetStringField(TEXT("message"));
        UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameInstance] 에러: %s"), *ErrorMsg);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameInstance] 알 수 없는 타입: %s"), *Type);
    }
}