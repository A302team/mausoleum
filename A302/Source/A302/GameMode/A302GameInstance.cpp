// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/A302GameInstance.h"
#include "Network/GameNetworkSubsystem.h"
#include "Dom/JsonObject.h"
#include "UI/LobbyWidget.h"
#include "UI/WaitingRoomWidget.h"
#include "Serialization/JsonSerializer.h"
#include "Kismet/GameplayStatics.h"
#include "GameMode/LobbyGameMode.h"
#include "Character/MyPlayerController.h"
#include "Network/LobbyConstants.h"

void UA302GameInstance::Init()
{
    Super::Init();

    GameNetworkSubsystem = Cast<UGameNetworkSubsystem>(GetSubsystemBase(UGameNetworkSubsystem::StaticClass()));
    if (GameNetworkSubsystem)
    {
        GameNetworkSubsystem->Connect(EProtocolType::WebSocket, GameNetworkSubsystem->GetLobbyURL());
        GameNetworkSubsystem->OnPacketReceived.AddDynamic(this, &UA302GameInstance::OnMessageReceived);
    }
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
        FString TargetURL = GameNetworkSubsystem->GetLobbyURL();
        UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameInstance] Blueprint requested connect to %s, but overriding with Config URL: %s"), *URL, *TargetURL);
        GameNetworkSubsystem->Connect(EProtocolType::WebSocket, TargetURL);
    }
}

void UA302GameInstance::SendToServer(const FString &Message)
{
    if (GameNetworkSubsystem)
    {
        GameNetworkSubsystem->SendPacket(EProtocolType::WebSocket, Message);
    }
}

void UA302GameInstance::OnMessageReceived(const FString &Message)
{
    UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 수신: %s"), *Message);

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject))
        return;

    FString Type = JsonObject->GetStringField(LobbyProtocol::KeyType);
    TSharedPtr<FJsonObject> Data = JsonObject->GetObjectField(LobbyProtocol::KeyData);

    if (Type == LobbyProtocol::ResRoomCreated)
    {
        CurrentRoomCode = Data->GetStringField(LobbyProtocol::KeyRoomCode);
        MyPlayerName = Data->GetStringField(LobbyProtocol::KeyPlayerName);
        bIsHost = true;
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 방 생성: %s"), *CurrentRoomCode);
        OnRoomCreated.Broadcast(CurrentRoomCode);
        ShowWaitingRoom(CurrentRoomCode);
    }
    else if (Type == LobbyProtocol::ResRoomJoined)
    {
        CurrentRoomCode = Data->GetStringField(LobbyProtocol::KeyRoomCode);
        MyPlayerName = Data->GetStringField(LobbyProtocol::KeyPlayerName);
        bIsHost = Data->GetBoolField(LobbyProtocol::KeyIsHost);
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 방 입장: %s"), *CurrentRoomCode);
        OnRoomJoined.Broadcast();
        ShowWaitingRoom(CurrentRoomCode);
    }
    else if (Type == LobbyProtocol::ResPlayerEntered)
    {
        FString EnteredName = Data->GetStringField(LobbyProtocol::KeyPlayerName);
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 플레이어 입장: %s"), *EnteredName);
        OnPlayerEntered.Broadcast(EnteredName);
    }
    else if (Type == LobbyProtocol::ResRoomList)
    {
        TArray<FRoomInfo> RoomList;
        const TArray<TSharedPtr<FJsonValue>> *Rooms;
        if (Data->TryGetArrayField(LobbyProtocol::KeyRooms, Rooms))
        {
            for (auto &Room : *Rooms)
            {
                FRoomInfo Info;
                Info.RoomCode = Room->AsObject()->GetStringField(LobbyProtocol::KeyRoomCode);
                Info.PlayerCount = Room->AsObject()->GetIntegerField(LobbyProtocol::KeyPlayerCount);
                RoomList.Add(Info);
            }
        }
        OnRoomListReceived.Broadcast(RoomList);
    }
    else if (Type == LobbyProtocol::ResPlayerReady)
    {
        FString ReadyName = Data->GetStringField(LobbyProtocol::KeyPlayerName);
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 플레이어 레디: %s"), *ReadyName);
        OnPlayerReady.Broadcast(ReadyName);
    }
    else if (Type == LobbyProtocol::ResPlayerLeft)
    {
        FString LeftName = Data->GetStringField(LobbyProtocol::KeyPlayerName);
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 플레이어 퇴장: %s"), *LeftName);
        OnPlayerLeft.Broadcast(LeftName);
    }
    else if (Type == LobbyProtocol::ResGameStarted)
    {
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 게임 시작!"));
        OnGameStarted.Broadcast();

        UWorld *MyWorld = GetWorld();
        if (!MyWorld)
            return;

        // 데디케이트 서버이면 무시
        if (MyWorld->GetNetMode() == NM_DedicatedServer)
            return;

        // 서버 주소 파싱 (WebSocket 서버가 보내준 주소)
        FString ServerIP = TEXT("j14a302.p.ssafy.io");
        int32 ServerPort = 47777;

        if (Data->HasField(TEXT("serverIP")))
            ServerIP = Data->GetStringField(TEXT("serverIP"));
        if (Data->HasField(TEXT("serverPort")))
            ServerPort = (int32)Data->GetNumberField(TEXT("serverPort"));

        FString TravelURL = FString::Printf(TEXT("%s:%d"), *ServerIP, ServerPort);
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 데디케이트 서버로 이동: %s"), *TravelURL);

        APlayerController *PC = MyWorld->GetFirstPlayerController();
        if (!PC)
            return;

        PC->ClientTravel(TravelURL, TRAVEL_Absolute);
    }
    else if (Type == LobbyProtocol::ResNicknameAvailable)
    {
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 닉네임 사용 가능"));
        OnNicknameAvailable.Broadcast();
    }
    else if (Type == LobbyProtocol::ResChatMessage)
    {
        FString PlayerName = Data->GetStringField(LobbyProtocol::KeyPlayerName);
        FString ChatMessage = Data->GetStringField(LobbyProtocol::KeyMessage);
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
    else if (Type == LobbyProtocol::ResError)
    {
        FString ErrorMsg = Data->GetStringField(LobbyProtocol::KeyMessage);
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
