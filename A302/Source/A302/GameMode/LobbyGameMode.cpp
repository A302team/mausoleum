// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyGameMode.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "UI/LobbyWidget.h"
#include "UI/WaitingRoomWidget.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"

ALobbyGameMode::ALobbyGameMode()
{
    WebSocketManager = CreateDefaultSubobject<UWebSocketManager>(TEXT("WebSocketManager"));
}

void ALobbyGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (!WebSocketManager)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode/LobbyGameMode] WebSocketManager가 null입니다!"));
        return;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        PC->bShowMouseCursor = true;
        PC->bEnableClickEvents = true;
        PC->SetInputMode(FInputModeUIOnly());
    }

    WebSocketManager->Connect(TEXT("ws://localhost:9001"));

    WebSocketManager->OnMessageReceived.AddDynamic(this, &ALobbyGameMode::OnMessageReceived);

    if (LobbyWidgetClass)
    {
        LobbyWidget = CreateWidget<ULobbyWidget>(GetWorld(), LobbyWidgetClass);
        if (LobbyWidget)
        {
            LobbyWidget->AddToViewport();
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[GameMode/LobbyGameMode] Begin Play."))
}

void ALobbyGameMode::SendToServer(const FString &Message)
{
    if (WebSocketManager)
    {
        WebSocketManager->SendMessage(Message);
    }
}

void ALobbyGameMode::OnMessageReceived(const FString &Message)
{
    UE_LOG(LogTemp, Log, TEXT("[GameMode/LobbyGameMode] ServerMessage : %s"), *Message);

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);

    if (!FJsonSerializer::Deserialize(Reader, JsonObject))
        return;

    FString Type = JsonObject->GetStringField(TEXT("type"));
    TSharedPtr<FJsonObject> Data = JsonObject->GetObjectField(TEXT("data"));

    if (Type == TEXT("room_joined"))
    {
        CurrentRoomCode = Data->GetStringField(TEXT("roomCode"));
        MyPlayerName = Data->GetStringField(TEXT("playerName"));
        bIsHost = Data->GetBoolField(TEXT("isHost"));
        UE_LOG(LogTemp, Log, TEXT("[GameMode/LobbyGameMode] Success enter the room. Code : %s"), *CurrentRoomCode);
        OnRoomJoined.Broadcast();

        ShowWaitingRoom(CurrentRoomCode);
    }
    else if (Type == TEXT("room_created"))
    {
        FString RoomCode = Data->GetStringField(TEXT("roomCode"));
        MyPlayerName = Data->GetStringField(TEXT("playerName"));
        bIsHost = true;
        CurrentRoomCode = RoomCode;
        UE_LOG(LogTemp, Log, TEXT("[GameMode/LobbyGameMode] Success create the room. Code : %s"), *RoomCode);
        OnRoomCreated.Broadcast(RoomCode);

        ShowWaitingRoom(RoomCode);
    }
    else if (Type == TEXT("room_list"))
    {
        TArray<FRoomInfo> RoomList;
        const TArray<TSharedPtr<FJsonValue>> *Rooms;

        if (Data->TryGetArrayField(TEXT("rooms"), Rooms))
        {
            for (auto &RoomValue : *Rooms)
            {
                TSharedPtr<FJsonObject> RoomObj = RoomValue->AsObject();
                FRoomInfo Info;
                Info.RoomCode = RoomObj->GetStringField(TEXT("roomCode"));
                Info.PlayerCount = RoomObj->GetIntegerField(TEXT("playerCount"));
                RoomList.Add(Info);
            }
        }

        UE_LOG(LogTemp, Log, TEXT("[GameMode/LobbyGameMode] Send Room List : %d"), RoomList.Num());
        OnRoomListReceived.Broadcast(RoomList);
    }
    else if (Type == TEXT("player_entered"))
    {
        FString EnteredName = Data->GetStringField(TEXT("playerName"));
        UE_LOG(LogTemp, Log, TEXT("[GameMode/LobbyGameMode] Enter Player : %s"), *EnteredName);
        OnPlayerEntered.Broadcast(EnteredName);
    }
    else if (Type == TEXT("player_ready"))
    {
        FString ReadyName = Data->GetStringField(TEXT("playerName"));
        UE_LOG(LogTemp, Log, TEXT("[GameMode/LobbyGameMode] %s Ready!"), *ReadyName);
        OnPlayerReady.Broadcast(ReadyName);
    }
    else if (Type == TEXT("game_started"))
    {
        UE_LOG(LogTemp, Log, TEXT("[GameMode/LobbyGameMode] Start the Game!"));
        OnGameStarted.Broadcast();

        UGameplayStatics::OpenLevel(this, TEXT("/Game/PersonalWorkSpace/sikk806/MyTestLevel"));
    }
    else if (Type == TEXT("nickname_available"))
    {
        UE_LOG(LogTemp, Log, TEXT("[GameMode/LobbyGameMode] You Can Use this Nickname."));
        OnNicknameAvailable.Broadcast();
    }
    else if (Type == TEXT("player_left"))
    {
        FString LeftName = Data->GetStringField(TEXT("playerName"));
        UE_LOG(LogTemp, Log, TEXT("[LobbyGameMode] 플레이어 퇴장: %s"), *LeftName);
        OnPlayerLeft.Broadcast(LeftName);
    }
    else if (Type == TEXT("error"))
    {
        FString ErrorMsg = Data->GetStringField(TEXT("message"));
        UE_LOG(LogTemp, Warning, TEXT("[GameMode/LobbyGameMode] Error : %s"), *ErrorMsg);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode/LobbyGameMode] Wrong Type : %s"), *Type);
    }
}

void ALobbyGameMode::ShowWaitingRoom(const FString &RoomCode)
{
    if (WaitingRoomWidgetClass)
    {
        WaitingRoomWidget = CreateWidget<UWaitingRoomWidget>(GetWorld(), WaitingRoomWidgetClass);
        if (WaitingRoomWidget)
        {
            WaitingRoomWidget->SetRoomCode(RoomCode);
            WaitingRoomWidget->AddToViewport();

            // 내 플레이어도 목록에 추가
            WaitingRoomWidget->OnPlayerEntered(MyPlayerName);
        }
    }
}