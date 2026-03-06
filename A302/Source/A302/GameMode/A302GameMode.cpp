// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/A302GameMode.h"
#include "Blueprint/UserWidget.h"
#include "GameMode/A302GameState.h"
#include "GameMode/A302PlayerState.h"
#include "GameMode/A302GameInstance.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "Manager/SpawnManager.h"
#include "Kismet/GameplayStatics.h"
#include "Network/GameNetworkSubsystem.h"
#include "UI/ChatWidget.h"
#include "Blueprint/UserWidget.h"


AA302GameMode::AA302GameMode()
{
    DefaultPawnClass = nullptr;
    // C++에서 StaticClass로 덮어씌우면 블루프린트로 설정한 입력 맵핑 등이 모두 날아갑니다.
    // 블루프린트 게임모드(BP_A302GameMode)에서 PlayerControllerClass 등을 직접 세팅해주세요.
}

void AA302GameMode::BeginPlay()
{
    Super::BeginPlay();

    UA302GameInstance *GI = Cast<UA302GameInstance>(GetGameInstance());
    if (GI)
    {
        CurrentRoomCode = GI->CurrentRoomCode;
        MyPlayerName = GI->MyPlayerName;
        bIsHost = GI->bIsHost;
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] RoomCode: %s, PlayerName: %s"),
               *CurrentRoomCode, *MyPlayerName);
    }

    TArray<AActor *> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnManager::StaticClass(), FoundActors);

    UGameNetworkSubsystem* GameNetworkSubsystem = GetGameInstance()->GetSubsystem<UGameNetworkSubsystem>();
    if (GameNetworkSubsystem)
    {
        // 인게임 진입 시엔 보이스/위치 동기화를 위한 UDP 통신만 연결합니다.
        GameNetworkSubsystem->Connect(EProtocolType::UDP, TEXT("127.0.0.1:9100"));
        GameNetworkSubsystem->OnPacketReceived.AddDynamic(this, &AA302GameMode::OnMessageReceived);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameMode] No GameNetworkSubsystem. You CAN'T use chat."));
    }

    if (FoundActors.Num() > 0)
    {
        SpawnManager = Cast<ASpawnManager>(FoundActors[0]);
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] Find SpawnManager."));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] Can't find SpawnManager"));
    }

    if (ChatWidgetClass)
    {
        ChatWidget = CreateWidget<UChatWidget>(GetWorld(), TSubclassOf<UUserWidget>(ChatWidgetClass));
        if (ChatWidget)
        {
            ChatWidget->AddToViewport();
            UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] ChatWidget 생성"));
        }
    }
    UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] BeginPlay"));
}

void AA302GameMode::PostLogin(APlayerController *NewPlayer)
{
    Super::PostLogin(NewPlayer);

    FInputModeGameOnly InputMode;
    NewPlayer->SetInputMode(InputMode);
    NewPlayer->bShowMouseCursor = false;

    UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] 플레이어 접속"));

    SpawnPlayer(NewPlayer);
}

void AA302GameMode::Logout(AController *Exiting)
{
    Super::Logout(Exiting);
    UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] 플레이어 퇴장"));
}

void AA302GameMode::SpawnPlayer(APlayerController *PlayerController)
{
    if (!SpawnManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameMode] No SpawnManager."));
        return;
    }

    FTransform SpawnTransform = SpawnManager->GetRandomPlayerSpawnTransform(CurrentStage);

    AMyCharacter *Character = GetWorld()->SpawnActor<AMyCharacter>(
        AMyCharacter::StaticClass(),
        SpawnTransform);

    if (Character)
    {
        PlayerController->Possess(Character);
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] Success SpawnPlayer."));
    }
}

void AA302GameMode::SendToServer(const FString &Message)
{
    UGameNetworkSubsystem* GameNetworkSubsystem = GetGameInstance()->GetSubsystem<UGameNetworkSubsystem>();
    if (GameNetworkSubsystem)
    {
        GameNetworkSubsystem->SendPacket(EProtocolType::WebSocket, Message);
    }
}

void AA302GameMode::OnMessageReceived(const FString &Message)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject))
        return;

    FString Type = JsonObject->GetStringField(TEXT("type"));
    TSharedPtr<FJsonObject> Data = JsonObject->GetObjectField(TEXT("data"));

    if (Type == TEXT("chat_message"))
    {
        FString PlayerName = Data->GetStringField(TEXT("playerName"));
        FString ChatMessage = Data->GetStringField(TEXT("message"));
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] Chatting Log >> %s: %s"), *PlayerName, *ChatMessage);
        OnInGameChatReceived.Broadcast(PlayerName, ChatMessage);
    }
}
