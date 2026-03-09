// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyGameMode.h"
#include "UI/LobbyWidget.h"
#include "UI/WaitingRoomWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameMode/A302GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Network/GameNetworkSubsystem.h"

ALobbyGameMode::ALobbyGameMode()
{
}

void ALobbyGameMode::BeginPlay()
{
    Super::BeginPlay();

    APlayerController *PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        PC->bShowMouseCursor = true;
        PC->bEnableClickEvents = true;
        PC->SetInputMode(FInputModeUIOnly());
    }

    UGameNetworkSubsystem* GameNetworkSubsystem = GetGameInstance()->GetSubsystem<UGameNetworkSubsystem>();
    if (GameNetworkSubsystem)
    {
        // 서버 환경(Dedicated/Listen)에서는 클라이언트 소켓 연결을 하지 않도록 보호
        if (GetNetMode() == NM_Client || GetNetMode() == NM_Standalone)
        {
            // 텍스트, 시그널링 이벤트 용 WebSocket (중앙 설정 주소 사용)
            GameNetworkSubsystem->Connect(EProtocolType::WebSocket, GameNetworkSubsystem->GetLobbyURL());
            
            // 보이스/위치 등 실시간/바이너리 데이터용 UDP (중앙 설정 주소 사용)
            GameNetworkSubsystem->Connect(EProtocolType::UDP, GameNetworkSubsystem->GetVoiceURL());

            GameNetworkSubsystem->OnPacketReceived.AddDynamic(this, &ALobbyGameMode::OnMessageReceived);
            UE_LOG(LogTemp, Log, TEXT("[GameMode/LobbyGameMode] Client connected to Servers."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode/LobbyGameMode] GameNetworkSubsystem이 유효하지 않습니다!"));
    }

    if (LobbyWidgetClass)
    {
        LobbyWidget = CreateWidget<ULobbyWidget>(GetWorld(), LobbyWidgetClass);
        if (LobbyWidget)
        {
            LobbyWidget->AddToViewport();
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[GameMode/LobbyGameMode] Begin Play."));
}

void ALobbyGameMode::SendToServer(const FString &Message)
{
    UGameNetworkSubsystem* GameNetworkSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UGameNetworkSubsystem>() : nullptr;
    if (GameNetworkSubsystem)
    {
        GameNetworkSubsystem->SendPacket(EProtocolType::WebSocket, Message);
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
        UA302GameInstance *GI = Cast<UA302GameInstance>(GetGameInstance());
        if (GI)
        {
            GI->CurrentRoomCode = CurrentRoomCode;
            GI->MyPlayerName = MyPlayerName;
            GI->bIsHost = bIsHost;
            UE_LOG(LogTemp, Log, TEXT("[GameMode/LobbyGameMode] GameInstance에 저장 - RoomCode: %s, PlayerName: %s"),
                   *CurrentRoomCode, *MyPlayerName);
        }
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
    else if (Type == TEXT("chat_message"))
    {
        FString PlayerName = Data->GetStringField(TEXT("playerName"));
        FString ChatMessage = Data->GetStringField(TEXT("message"));
        UE_LOG(LogTemp, Log, TEXT("[Chat] %s: %s"), *PlayerName, *ChatMessage);
        OnChatMessageReceived.Broadcast(PlayerName, ChatMessage);
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

            // 로비 위젯 입력 막기
            if (LobbyWidget)
            {
                LobbyWidget->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }
}
