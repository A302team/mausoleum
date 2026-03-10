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
#include "UI/ChatWidget.h"
#include "Blueprint/UserWidget.h"
#include "Network/LobbyConstants.h"

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

    if (FoundActors.Num() > 0)
    {
        SpawnManager = Cast<ASpawnManager>(FoundActors[0]);
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] Find SpawnManager."));
    }

    // 서버에서는 위젯 생성 안함
    if (GetNetMode() == NM_DedicatedServer || GetNetMode() == NM_ListenServer)
    {
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] BeginPlay"));
        return;
    }

    if (ChatWidgetClass)
    {
        APlayerController *PC = UGameplayStatics::GetPlayerController(this, 0);
        if (PC && PC->IsLocalPlayerController())
        {
            ChatWidget = CreateWidget<UChatWidget>(PC, TSubclassOf<UUserWidget>(ChatWidgetClass));
            if (ChatWidget)
            {
                ChatWidget->AddToViewport();
                UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] ChatWidget 생성"));
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] BeginPlay"));
}

void AA302GameMode::PostLogin(APlayerController *NewPlayer)
{
    Super::PostLogin(NewPlayer);

    UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] 플레이어 접속 - NetMode: %d"),
           (int32)GetNetMode());

    // DedicatedServer 또는 ListenServer에서만 스폰
    if (GetNetMode() != NM_DedicatedServer && GetNetMode() != NM_ListenServer)
    {
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] 서버가 아니므로 스폰 안함"));
        return;
    }

    FInputModeGameOnly InputMode;
    NewPlayer->SetInputMode(InputMode);
    NewPlayer->bShowMouseCursor = false;

    if (!SpawnManager)
    {
        FTimerHandle Handle;
        GetWorldTimerManager().SetTimer(Handle, [this, NewPlayer]()
                                        { SpawnPlayer(NewPlayer); }, 0.5f, false);
    }
    else
    {
        SpawnPlayer(NewPlayer);
    }
}

void AA302GameMode::Logout(AController *Exiting)
{
    Super::Logout(Exiting);
    UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] 플레이어 퇴장"));
}

void AA302GameMode::SpawnPlayer(APlayerController *PlayerController)
{
    if (!SpawnManager)
        return;

    FTransform SpawnTransform = SpawnManager->GetRandomPlayerSpawnTransform(CurrentStage);

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    UClass *SpawnClass = CharacterClass
                             ? (UClass *)CharacterClass
                             : AMyCharacter::StaticClass();

    AMyCharacter *Character = GetWorld()->SpawnActor<AMyCharacter>(
        SpawnClass, SpawnTransform, SpawnParams);

    if (Character)
    {
        Character->SetReplicates(true);
        Character->SetReplicateMovement(true);
        PlayerController->Possess(Character);
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] SpawnPlayer 성공"));
    }
}

void AA302GameMode::SendToServer(const FString &Message)
{
}

void AA302GameMode::OnMessageReceived(const FString &Message)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject))
        return;

    FString Type = JsonObject->GetStringField(LobbyProtocol::KeyType);
    TSharedPtr<FJsonObject> Data = JsonObject->GetObjectField(LobbyProtocol::KeyData);

    if (Type == LobbyProtocol::ResChatMessage)
    {
        FString PlayerName = Data->GetStringField(LobbyProtocol::KeyPlayerName);
        FString ChatMessage = Data->GetStringField(LobbyProtocol::KeyMessage);
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] Chatting Log >> %s: %s"), *PlayerName, *ChatMessage);
        OnInGameChatReceived.Broadcast(PlayerName, ChatMessage);
    }
}
