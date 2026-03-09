// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/A302GameInstance.h"
#include "Dom/JsonObject.h"
#include "UI/LobbyWidget.h"
#include "UI/WaitingRoomWidget.h"
#include "Serialization/JsonSerializer.h"
#include "Kismet/GameplayStatics.h"
#include "GameMode/LobbyGameMode.h"
#include "Character/MyPlayerController.h"
#include "Network/GameNetworkSubsystem.h"
#include "Network/WebSocketHandler.h"

void UA302GameInstance::Init()
{
    Super::Init();

    GameNetworkSubsystem = NewObject<UGameNetworkSubsystem>(this, TEXT("GameNetworkSubsystem"));
    if (!GameNetworkSubsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameInstance] No GameNetworkSubsystem"));
    }
    WebSocketHandler = NewObject<UWebSocketHandler>(this, TEXT("WebSocketHandler"));
    WebSocketHandler->OnMessageReceived.AddDynamic(this, &UA302GameInstance::OnMessageReceived);

    GameNetworkSubsystem->Connect(EProtocolType::WebSocket, TEXT("ws://j14a302.p.ssafy.io:8001"));

    UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] Init - WebSocket 연결 시도"));

    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UA302GameInstance::OnMapLoaded);
}

void UA302GameInstance::OnMapLoaded(UWorld *LoadedWorld)
{
    if (!LoadedWorld)
        return;
    if (LoadedWorld->GetNetMode() == NM_DedicatedServer)
        return;
    if (LoadedWorld != GetWorld())
        return;

    FString MapName = LoadedWorld->GetMapName();
    UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 맵 로드됨: %s"), *MapName);

    // 인게임 레벨로 이동 시 로비 UI 제거
    if (MapName.Contains(TEXT("MyTestLevel")))
    {
        if (LobbyWidget)
        {
            LobbyWidget->RemoveFromParent();
            LobbyWidget = nullptr;
        }
        if (WaitingRoomWidget)
        {
            WaitingRoomWidget->RemoveFromParent();
            WaitingRoomWidget = nullptr;
        }
        return;
    }

    // 로비 레벨에서만 로비 위젯 생성
    if (!MapName.Contains(TEXT("testLevel")))
        return;

    FTimerHandle Handle;
    LoadedWorld->GetTimerManager().SetTimer(Handle, [this, LoadedWorld]()
                                            {
        if (!LoadedWorld) return;

        APlayerController* PC = LoadedWorld->GetFirstPlayerController();
        if (!PC)
        {
            UE_LOG(LogTemp, Error, TEXT("[GameMode/A302GameInstance] PC가 NULL입니다!"));
            return;
        }

        PC->bShowMouseCursor = true;
        PC->bEnableClickEvents = true;
        PC->SetInputMode(FInputModeUIOnly());

        if (LobbyWidgetClass)
        {
            LobbyWidget = CreateWidget<ULobbyWidget>(PC, LobbyWidgetClass);
            if (LobbyWidget)
            {
                LobbyWidget->AddToViewport();
                UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] LobbyWidget 생성 성공"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[GameMode/A302GameInstance] LobbyWidgetClass가 NULL입니다!"));
        } }, 0.1f, false);
}

void UA302GameInstance::ConnectToServer(const FString &URL)
{
    if (GameNetworkSubsystem)
    {
        GameNetworkSubsystem->Connect(EProtocolType::WebSocket, URL);
    }
}

void UA302GameInstance::SendToServer(const FString &Message)
{
    if (WebSocketHandler)
    {
        WebSocketHandler->SendMessage(Message);
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
        ShowWaitingRoom(CurrentRoomCode);
    }
    else if (Type == TEXT("room_joined"))
    {
        CurrentRoomCode = Data->GetStringField(TEXT("roomCode"));
        MyPlayerName = Data->GetStringField(TEXT("playerName"));
        bIsHost = Data->GetBoolField(TEXT("isHost"));
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 방 입장: %s"), *CurrentRoomCode);
        OnRoomJoined.Broadcast();
        ShowWaitingRoom(CurrentRoomCode);
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

        UWorld *MyWorld = GetWorld();
        if (!MyWorld)
            return;

        ENetMode NetMode = MyWorld->GetNetMode();
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 내 NetMode: %d"), (int32)NetMode);

        APlayerController *PC = MyWorld->GetFirstPlayerController();
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] PC: %s"), *GetNameSafe(PC));

        AMyPlayerController *MyPC = Cast<AMyPlayerController>(PC);
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] MyPC: %s"), *GetNameSafe(MyPC));

        // if (MyPC)
        // {
        //     UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] IsLocalController: %d"), (int32)MyPC->IsLocalController());
        //     if (MyPC->IsLocalController())
        //     {
        //         UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] ServerRequestGameStart 호출!"));
        //         MyPC->ServerRequestGameStart();
        //     }
        // }
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

void UA302GameInstance::ShowWaitingRoom(const FString &RoomCode)
{
    UWorld *World = GetWorld();
    if (!World)
        return;

    APlayerController *PC = UGameplayStatics::GetPlayerController(World, 0);
    if (!PC)
        return;

    if (WaitingRoomWidgetClass)
    {
        WaitingRoomWidget = CreateWidget<UWaitingRoomWidget>(PC, WaitingRoomWidgetClass);
        if (WaitingRoomWidget)
        {
            WaitingRoomWidget->SetRoomCode(RoomCode);
            WaitingRoomWidget->AddToViewport();
            WaitingRoomWidget->OnPlayerEntered(MyPlayerName);

            if (LobbyWidget)
            {
                LobbyWidget->SetVisibility(ESlateVisibility::Collapsed);
            }

            UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] WaitingRoomWidget 생성 성공"));
        }
    }
}