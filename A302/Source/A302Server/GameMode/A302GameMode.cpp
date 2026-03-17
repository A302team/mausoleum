// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/A302GameMode.h"
#include "GameMode/A302GameState.h"
#include "GameMode/A302PlayerState.h"
#include "Room/RoomMembershipRegistry.h"
#include "Phase/A302ServerPhaseSubsystem.h"
#include "Room/A302RoomRuntimeSubsystem.h"
#include "Reward/A302ServerEventResolver.h"
#include "Manager/SpawnManager.h"
#include "Network/GameServerBackendSubsystem.h"
#include "Network/A302NetworkEndpointConfig.h"
#include "Character/MyCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Network/LobbyConstants.h"
#include "A302GameplayGuards.h"
#include "UObject/ConstructorHelpers.h"
#include "Room/RoomScopeRules.h"
#include "Room/RoomWorldOffset.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

AA302GameMode::AA302GameMode()
{
    CharacterClass = AMyCharacter::StaticClass();
    static ConstructorHelpers::FClassFinder<ACharacter> CharacterBPClass(TEXT("/Game/WorkSpace/Character/BP_MyCharacter"));
    if (CharacterBPClass.Succeeded())
    {
        CharacterClass = CharacterBPClass.Class;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameMode] Failed to load BP_MyCharacter. fallback=%s"), *GetNameSafe(CharacterClass.Get()));
    }

    DefaultPawnClass = nullptr;
    PlayerControllerClass = APlayerController::StaticClass();
    static ConstructorHelpers::FClassFinder<APlayerController> PlayerControllerBPClass(TEXT("/Game/WorkSpace/Character/BP_MyPlayerController"));
    if (PlayerControllerBPClass.Succeeded())
    {
        PlayerControllerClass = PlayerControllerBPClass.Class;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameMode] Failed to load BP_MyPlayerController. fallback=%s"), *GetNameSafe(PlayerControllerClass.Get()));
    }

    PlayerStateClass = AA302PlayerState::StaticClass();
    GameStateClass = AA302GameState::StaticClass();
    bUseSeamlessTravel = true;
    bStartPlayersAsSpectators = true;
}

void AA302GameMode::BeginPlay()
{
    Super::BeginPlay();
    DefaultPawnClass = nullptr;

    if (!RoomMembershipRegistry)
    {
        RoomMembershipRegistry = NewObject<URoomMembershipRegistry>(this);
    }

    EnsureSpawnManager();

    if (GetNetMode() == NM_DedicatedServer)
    {
        if (UGameInstance* GameInstance = GetGameInstance())
        {
            BackendSubsystem = GameInstance->GetSubsystem<UGameServerBackendSubsystem>();
            PhaseSubsystem = GameInstance->GetSubsystem<UA302ServerPhaseSubsystem>();
            RoomRuntimeSubsystem = GameInstance->GetSubsystem<UA302RoomRuntimeSubsystem>();
            if (PhaseSubsystem)
            {
                PhaseSubsystem->OnRoomPhaseChanged.RemoveDynamic(this, &AA302GameMode::HandleRoomPhaseChanged);
                PhaseSubsystem->OnRoomPhaseChanged.AddDynamic(this, &AA302GameMode::HandleRoomPhaseChanged);
            }
            if (RoomRuntimeSubsystem)
            {
                RoomRuntimeSubsystem->OnRoomLevelReady().RemoveAll(this);
                RoomRuntimeSubsystem->OnRoomLevelReady().AddUObject(this, &AA302GameMode::HandleRoomLevelReady);

                const TWeakObjectPtr<AA302GameMode> WeakThis(this);
                RoomRuntimeSubsystem->SetRoomPopulationResolver([WeakThis](const FString& InRoomCode) -> int32
                {
                    if (!WeakThis.IsValid() || !WeakThis->RoomMembershipRegistry || !WeakThis->GetWorld())
                    {
                        return 0;
                    }

                    return WeakThis->RoomMembershipRegistry->CountPlayersInRoom(WeakThis->GetWorld(), InRoomCode);
                });
            }
            if (BackendSubsystem)
            {
                BackendSubsystem->OnPacketReceived.AddDynamic(this, &AA302GameMode::OnMessageReceived);
            }
        }
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[GameMode/A302GameMode] Class setup. CharacterClass=%s PlayerControllerClass=%s"),
        *GetNameSafe(CharacterClass.Get()),
        *GetNameSafe(PlayerControllerClass.Get())
    );

    UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] BeginPlay"));
}

void AA302GameMode::PostLogin(APlayerController *NewPlayer)
{
    Super::PostLogin(NewPlayer);
    if (RoomMembershipRegistry)
    {
        RoomMembershipRegistry->AssignPlayerToRoom(NewPlayer);
    }
    UpdatePlayerGameplayFlag(NewPlayer, IsPlayerGameplayEnabled(NewPlayer));

    UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] 플레이어 접속 - NetMode: %d"),
           (int32)GetNetMode());

    // DedicatedServer 또는 ListenServer에서만 스폰
    if (GetNetMode() != NM_DedicatedServer && GetNetMode() != NM_ListenServer)
    {
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] 서버가 아니므로 스폰 안함"));
        return;
    }

	if (!IsPlayerGameplayEnabled(NewPlayer))
	{
		const FString WaitingRoomCode = RoomMembershipRegistry
			? RoomMembershipRegistry->GetPlayerRoomCode(NewPlayer)
			: FString();
        const FString NormalizedWaitingRoomCode = A302RoomScope::NormalizeRoomCode(WaitingRoomCode);

		UE_LOG(
			LogTemp,
			Log,
			TEXT("[GameMode/A302GameMode] Room not ready. keep controller waiting. player=%s room=%s active=%d ready=%d"),
			*GetNameSafe(NewPlayer),
			*WaitingRoomCode,
			ActiveGameplayRooms.Contains(NormalizedWaitingRoomCode) ? 1 : 0,
            ReadyGameplayRooms.Contains(NormalizedWaitingRoomCode) ? 1 : 0
		);
		return;
	}

    QueueSpawnPlayer(NewPlayer);
}

void AA302GameMode::Logout(AController *Exiting)
{
	FString LeavingRoomCode;
    if (APlayerController* ExitingPC = Cast<APlayerController>(Exiting))
    {
        if (RoomMembershipRegistry)
        {
            LeavingRoomCode = RoomMembershipRegistry->GetPlayerRoomCode(ExitingPC);
        }

        if (RoomMembershipRegistry)
        {
            RoomMembershipRegistry->ClearPendingRoomCode(ExitingPC);
        }
    }

    Super::Logout(Exiting);

	LeavingRoomCode = A302RoomScope::NormalizeRoomCode(LeavingRoomCode);
	if (!LeavingRoomCode.IsEmpty() && RoomMembershipRegistry && GetWorld())
    {
        if (RoomMembershipRegistry->CountPlayersInRoom(GetWorld(), LeavingRoomCode) <= 0)
        {
            ActiveGameplayRooms.Remove(LeavingRoomCode);
            ReadyGameplayRooms.Remove(LeavingRoomCode);
            if (PhaseSubsystem)
            {
                PhaseSubsystem->StopRoomPhaseTimeline(LeavingRoomCode);
            }
            if (RoomRuntimeSubsystem)
            {
                RoomRuntimeSubsystem->StopRoomRuntime(LeavingRoomCode);
            }

            UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] Room lifecycle cleared after last logout. room=%s"), *LeavingRoomCode);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] 플레이어 퇴장"));
}

bool AA302GameMode::EnsureSpawnManager()
{
    if (IsValid(SpawnManager))
    {
        return true;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    SpawnManager = ASpawnManager::FindOrSpawn(World);
    if (IsValid(SpawnManager))
    {
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] SpawnManager ready: %s"), *GetNameSafe(SpawnManager));
        return true;
    }

    UE_LOG(LogTemp, Error, TEXT("[GameMode/A302GameMode] Failed to acquire SpawnManager."));
    return false;
}

void AA302GameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
    if (!NewPlayer)
    {
        return;
    }

    if (GetNetMode() != NM_DedicatedServer && GetNetMode() != NM_ListenServer)
    {
        return;
    }

    Super::HandleStartingNewPlayer_Implementation(NewPlayer);

    if (RoomMembershipRegistry)
    {
        RoomMembershipRegistry->AssignPlayerToRoom(NewPlayer);
    }
    UpdatePlayerGameplayFlag(NewPlayer, IsPlayerGameplayEnabled(NewPlayer));

    if (!IsPlayerGameplayEnabled(NewPlayer))
    {
        return;
    }

    QueueSpawnPlayer(NewPlayer);
}

APawn* AA302GameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
    UE_LOG(
        LogTemp,
        Verbose,
        TEXT("[GameMode/A302GameMode] Skip default pawn spawn. player=%s start=%s"),
        *GetNameSafe(NewPlayer),
        *GetNameSafe(StartSpot)
    );
    return nullptr;
}

FString AA302GameMode::InitNewPlayer(
    APlayerController* NewPlayerController,
    const FUniqueNetIdRepl& UniqueId,
    const FString& Options,
    const FString& Portal
)
{
    const FString ErrorMessage = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

    if (NewPlayerController)
    {
        if (RoomMembershipRegistry)
        {
            RoomMembershipRegistry->RegisterPendingRoomCode(NewPlayerController, Options);
        }
    }

    return ErrorMessage;
}

bool AA302GameMode::TryHandlePersonalEventReward(ACharacter* InstigatorCharacter, AActor* InteractedActor, const URewardDefinition* RewardDefinition)
{
    if (!InstigatorCharacter || !A302GameplayGuards::IsGameplayEnabledCharacter(InstigatorCharacter))
    {
        UE_LOG(LogTemp, Verbose, TEXT("[GameMode/A302GameMode] Ignore personal event reward before room gameplay start. player=%s room=%s"),
            *GetNameSafe(InstigatorCharacter),
            InstigatorCharacter->GetPlayerState<AA302PlayerState>() ? *InstigatorCharacter->GetPlayerState<AA302PlayerState>()->GetRoomCode() : TEXT("none"));
        return false;
    }

    if (const AA302PlayerState* InstigatorState = InstigatorCharacter->GetPlayerState<AA302PlayerState>())
    {
        if (!IsRoomGameplayActive(InstigatorState->GetRoomCode()))
        {
            return false;
        }
    }

    return FA302ServerEventResolver::TryHandlePersonalEventReward(InstigatorCharacter, InteractedActor, RewardDefinition);
}

bool AA302GameMode::TryHandleGroupEventReward(ACharacter* InstigatorCharacter, AActor* InteractedActor, const URewardDefinition* RewardDefinition)
{
    if (!InstigatorCharacter || !A302GameplayGuards::IsGameplayEnabledCharacter(InstigatorCharacter))
    {
        UE_LOG(LogTemp, Verbose, TEXT("[GameMode/A302GameMode] Ignore group event reward before room gameplay start. player=%s room=%s"),
            *GetNameSafe(InstigatorCharacter),
            InstigatorCharacter->GetPlayerState<AA302PlayerState>() ? *InstigatorCharacter->GetPlayerState<AA302PlayerState>()->GetRoomCode() : TEXT("none"));
        return false;
    }

    if (const AA302PlayerState* InstigatorState = InstigatorCharacter->GetPlayerState<AA302PlayerState>())
    {
        if (!IsRoomGameplayActive(InstigatorState->GetRoomCode()))
        {
            return false;
        }
    }

    return FA302ServerEventResolver::TryHandleGroupEventReward(InstigatorCharacter, InteractedActor, RewardDefinition);
}

void AA302GameMode::HandlePrepareGame(const TSharedPtr<FJsonObject>& Data)
{
    if (!BackendSubsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameMode] prepare_game ignored: BackendSubsystem is null"));
        return;
    }

    if (!Data.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameMode] prepare_game ignored: data is invalid"));
        return;
    }

	FString RoomCode;
	if (!Data->TryGetStringField(BackendProtocol::KeyRoomCode, RoomCode) || RoomCode.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameMode] prepare_game ignored: roomCode missing"));
		return;
	}
	RoomCode = A302RoomScope::NormalizeRoomCode(RoomCode);

	StartRoomGameplay(RoomCode);
	const int32 PlayersInRoom = (RoomMembershipRegistry && GetWorld())
		? RoomMembershipRegistry->CountPlayersInRoom(GetWorld(), RoomCode)
		: 0;

    const TSharedPtr<FJsonObject> AckData = MakeShared<FJsonObject>();
    AckData->SetStringField(BackendProtocol::KeyRoomCode, RoomCode);
    AckData->SetStringField(BackendProtocol::KeyServerId, BackendSubsystem->DedicatedServerId);
    AckData->SetStringField(BackendProtocol::KeyGameHost, FA302NetworkEndpointConfig::GetGameServerHost());
    AckData->SetNumberField(BackendProtocol::KeyGamePort, FA302NetworkEndpointConfig::GameServerPort);

    const TSharedPtr<FJsonObject> AckMessage = MakeShared<FJsonObject>();
    AckMessage->SetStringField(LobbyProtocol::KeyDomain, BackendProtocol::Domain);
    AckMessage->SetStringField(LobbyProtocol::KeyType, BackendProtocol::ReqDedicatedReady);
    AckMessage->SetObjectField(LobbyProtocol::KeyData, AckData);

    FString Payload;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
    if (!FJsonSerializer::Serialize(AckMessage.ToSharedRef(), Writer))
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameMode] Failed to serialize dedicated_ready payload"));
        return;
    }

    BackendSubsystem->SendPacket(Payload);

    UE_LOG(
        LogTemp,
        Log,
		TEXT("[GameMode/A302GameMode] prepare_game handled. room=%s ack=dedi_ready host=%s port=%d"),
		*RoomCode,
		*FA302NetworkEndpointConfig::GetGameServerHost(),
		FA302NetworkEndpointConfig::GameServerPort
	);
	UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] room=%s current_players=%d"), *RoomCode, PlayersInRoom);

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[GameMode/A302GameMode] Physical room partition mode active. Keep single world instance. room=%s"),
        *RoomCode
    );
}

void AA302GameMode::HandleRoomPhaseChanged(const FString& RoomCode, EGamePhase NewPhase)
{
	const FString NormalizedRoomCode = A302RoomScope::NormalizeRoomCode(RoomCode);
	if (NormalizedRoomCode.IsEmpty())
	{
		return;
	}

    if (NewPhase != EGamePhase::Ended)
    {
        return;
    }

	ActiveGameplayRooms.Remove(NormalizedRoomCode);
    ReadyGameplayRooms.Remove(NormalizedRoomCode);
	if (!RoomMembershipRegistry || !GetWorld())
	{
		return;
	}

	TArray<APlayerController*> RoomPlayers;
	RoomMembershipRegistry->GatherPlayersInRoom(GetWorld(), NormalizedRoomCode, RoomPlayers);
	for (APlayerController* RoomPlayer : RoomPlayers)
	{
		UpdatePlayerGameplayFlag(RoomPlayer, false);
	}
}

void AA302GameMode::HandleRoomLevelReady(const FString& RoomCode)
{
	const FString NormalizedRoomCode = A302RoomScope::NormalizeRoomCode(RoomCode);
	if (NormalizedRoomCode.IsEmpty() || !ActiveGameplayRooms.Contains(NormalizedRoomCode))
	{
		return;
	}

    ReadyGameplayRooms.Add(NormalizedRoomCode);
	SpawnPlayersInRoom(NormalizedRoomCode);
}

bool AA302GameMode::IsRoomGameplayActive(const FString& RoomCode) const
{
	const FString NormalizedRoomCode = A302RoomScope::NormalizeRoomCode(RoomCode);
	if (NormalizedRoomCode.IsEmpty() || !ActiveGameplayRooms.Contains(NormalizedRoomCode))
	{
		return false;
	}

	if (PhaseSubsystem)
	{
		return PhaseSubsystem->IsRoomPhaseActive(NormalizedRoomCode);
	}

	return true;
}

bool AA302GameMode::IsPlayerGameplayEnabled(const APlayerController* PlayerController) const
{
    if (!RoomMembershipRegistry || !PlayerController)
    {
        return false;
    }

    const FString RoomCode = A302RoomScope::NormalizeRoomCode(RoomMembershipRegistry->GetPlayerRoomCode(PlayerController));
    if (RoomCode.IsEmpty())
    {
        return false;
    }

    return IsRoomGameplayActive(RoomCode) && ReadyGameplayRooms.Contains(RoomCode);
}

void AA302GameMode::UpdatePlayerGameplayFlag(APlayerController* PlayerController, bool bEnabled) const
{
    if (!PlayerController)
    {
        return;
    }

    if (AA302PlayerState* A302PlayerState = PlayerController->GetPlayerState<AA302PlayerState>())
    {
        A302PlayerState->SetGameplayEnabled(bEnabled);
    }
}

void AA302GameMode::SpawnPlayersInRoom(const FString& RoomCode)
{
	const FString NormalizedRoomCode = A302RoomScope::NormalizeRoomCode(RoomCode);
	if (NormalizedRoomCode.IsEmpty() || !RoomMembershipRegistry || !ReadyGameplayRooms.Contains(NormalizedRoomCode))
	{
		return;
	}

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

	if (RoomRuntimeSubsystem)
	{
		RoomRuntimeSubsystem->TouchRoom(NormalizedRoomCode);
	}

	TArray<APlayerController*> RoomPlayers;
	RoomMembershipRegistry->GatherPlayersInRoom(World, NormalizedRoomCode, RoomPlayers);
	for (APlayerController* RoomPlayer : RoomPlayers)
	{
		UpdatePlayerGameplayFlag(RoomPlayer, true);
		QueueSpawnPlayer(RoomPlayer);
	}
}

void AA302GameMode::QueueSpawnPlayer(APlayerController* PlayerController)
{
    if (!PlayerController)
    {
        return;
    }

    if (!IsPlayerGameplayEnabled(PlayerController))
    {
        return;
    }

    FString PlayerRoomCode;
    if (RoomMembershipRegistry)
    {
        PlayerRoomCode = RoomMembershipRegistry->GetPlayerRoomCode(PlayerController);
    }

    PlayerRoomCode = A302RoomScope::NormalizeRoomCode(PlayerRoomCode);
    if (PlayerRoomCode.IsEmpty())
    {
        if (const AA302PlayerState* A302PlayerState = PlayerController->GetPlayerState<AA302PlayerState>())
        {
            PlayerRoomCode = A302RoomScope::NormalizeRoomCode(A302PlayerState->GetRoomCode());
        }
    }

    if (!EnsureSpawnManager() || !SpawnManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameMode] SpawnManager unavailable. queue skipped. room=%s"), *PlayerRoomCode);
        return;
    }

    SpawnManager->QueueSpawnAndPossessPlayer(
        PlayerController,
        CharacterClass,
        CurrentStage,
        PlayerRoomCode,
        ClientRoomStreamWarmupSeconds
    );
}

void AA302GameMode::StartRoomGameplay(const FString& RoomCode)
{
	const FString NormalizedRoomCode = A302RoomScope::NormalizeRoomCode(RoomCode);
	if (NormalizedRoomCode.IsEmpty())
	{
		return;
	}

	ActiveGameplayRooms.Add(NormalizedRoomCode);
	if (PhaseSubsystem)
	{
		PhaseSubsystem->StartRoomPhaseTimeline(NormalizedRoomCode);
	}

    bool bReadyToSpawn = true;
	if (RoomRuntimeSubsystem)
	{
		bReadyToSpawn = RoomRuntimeSubsystem->PrepareRoom(NormalizedRoomCode);
	}

	if (bReadyToSpawn)
	{
        ReadyGameplayRooms.Add(NormalizedRoomCode);
		SpawnPlayersInRoom(NormalizedRoomCode);
	}
	else
	{
        ReadyGameplayRooms.Remove(NormalizedRoomCode);
		UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] Room level loading in progress. Spawn deferred. room=%s"), *NormalizedRoomCode);
	}
}

void AA302GameMode::OnMessageReceived(const FString &Message)
{
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject))
        return;

    FString Type;
    if (!JsonObject->TryGetStringField(LobbyProtocol::KeyType, Type))
    {
        return;
    }

    const TSharedPtr<FJsonObject>* DataPtr = nullptr;
    if (!JsonObject->TryGetObjectField(LobbyProtocol::KeyData, DataPtr) || DataPtr == nullptr || !DataPtr->IsValid())
    {
        return;
    }
    const TSharedPtr<FJsonObject> Data = *DataPtr;

    FString Domain;
    JsonObject->TryGetStringField(LobbyProtocol::KeyDomain, Domain);
    if (Domain == BackendProtocol::Domain)
    {
        if (Type == BackendProtocol::ReqPrepareGame)
        {
            HandlePrepareGame(Data);
            return;
        }

        UE_LOG(LogTemp, Verbose, TEXT("[GameMode/A302GameMode] Unknown backend packet type: %s"), *Type);
        return;
    }

    if (Type == LobbyProtocol::ResChatMessage)
    {
        const FString MessageRoomCode = A302RoomScope::ExtractRoomCode(Data);
        if (MessageRoomCode.IsEmpty())
        {
            UE_LOG(
                LogTemp,
                Verbose,
                TEXT("[GameMode/A302GameMode] Ignore chat without roomCode. payload=%s"),
                *Message
            );
            return;
        }

        if (!RoomMembershipRegistry || !GetWorld() || RoomMembershipRegistry->CountPlayersInRoom(GetWorld(), MessageRoomCode) <= 0)
        {
            UE_LOG(LogTemp, Verbose, TEXT("[GameMode/A302GameMode] Ignore chat for empty room. room=%s"), *MessageRoomCode);
            return;
        }

        if (RoomRuntimeSubsystem)
        {
            RoomRuntimeSubsystem->TouchRoom(MessageRoomCode);
        }

        FString PlayerName = Data->GetStringField(LobbyProtocol::KeyPlayerName);
        FString ChatMessage = Data->GetStringField(LobbyProtocol::KeyMessage);
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] Chatting Log >> room=%s %s: %s"), *MessageRoomCode, *PlayerName, *ChatMessage);
        OnInGameChatReceived.Broadcast(PlayerName, ChatMessage);
    }
}

