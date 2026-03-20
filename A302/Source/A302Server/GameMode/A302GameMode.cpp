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
#include "Character/MyPlayerController.h"
#include "Character/Components/MaliceComponent.h"
#include "GameData/RewardDefinition.h"
#include "Subsystem/A302ServerPlayerSubsystem.h"
#include "Network/A302ServerBackendRouter.h"
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

namespace
{
    constexpr float ResultScreenDisplaySeconds = 5.0f;

    FText BuildResultRoleText(const AA302PlayerState* PlayerState)
    {
        if (!PlayerState)
        {
            return NSLOCTEXT("A302GameMode", "UnknownRole", "알 수 없음");
        }

        return PlayerState->PlayerRole == EPlayerRole::Evil
            ? NSLOCTEXT("A302GameMode", "RoleEvil", "악인")
            : NSLOCTEXT("A302GameMode", "RoleInnocent", "선인");
    }

    FText BuildResultTitle(const AA302PlayerState* PlayerState, const AMyCharacter* Character)
    {
        const UMaliceComponent* MaliceComponent = Character ? Character->FindComponentByClass<UMaliceComponent>() : nullptr;
        if (PlayerState && PlayerState->bIsEscaped)
        {
            return NSLOCTEXT("A302GameMode", "EscapedTitle", "탈출 성공");
        }

        if (MaliceComponent && MaliceComponent->bMaliceEnding)
        {
            return NSLOCTEXT("A302GameMode", "MaliceEndingTitle", "악인 엔딩");
        }

        if (MaliceComponent && MaliceComponent->bNiceEnding)
        {
            return NSLOCTEXT("A302GameMode", "NiceEndingTitle", "선인 엔딩");
        }

        if (PlayerState && !PlayerState->bIsAlive)
        {
            return NSLOCTEXT("A302GameMode", "EliminatedTitle", "사망");
        }

        return NSLOCTEXT("A302GameMode", "ResultTitle", "게임 결과");
    }

    FText BuildResultDescription(const AA302PlayerState* PlayerState, const AMyCharacter* Character)
    {
        const UMaliceComponent* MaliceComponent = Character ? Character->FindComponentByClass<UMaliceComponent>() : nullptr;
        const FText RoleText = BuildResultRoleText(PlayerState);
        const FText StatusText = PlayerState && PlayerState->bIsEscaped
            ? NSLOCTEXT("A302GameMode", "StatusEscaped", "탈출함")
            : (PlayerState && !PlayerState->bIsAlive
                ? NSLOCTEXT("A302GameMode", "StatusDead", "사망함")
                : NSLOCTEXT("A302GameMode", "StatusSurvived", "생존"));
        const int32 MaliceCount = MaliceComponent ? MaliceComponent->GetMaliceCount() : 0;

        return FText::Format(
            NSLOCTEXT("A302GameMode", "ResultDescription", "역할: {0}\n상태: {1}\nMalice: {2}"),
            RoleText,
            StatusText,
            FText::AsNumber(MaliceCount)
        );
    }
}

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
    PlayerControllerClass = AMyPlayerController::StaticClass();
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

    if (!BackendRouter)
    {
        BackendRouter = NewObject<UA302ServerBackendRouter>(this);
        BackendRouter->Initialize(this);
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
    
    if (UA302ServerPlayerSubsystem* PlayerSubsystem = GetWorld()->GetSubsystem<UA302ServerPlayerSubsystem>())
    {
        PlayerSubsystem->HandlePlayerLogin(NewPlayer);
    }
}

void AA302GameMode::Logout(AController *Exiting)
{
    if (UA302ServerPlayerSubsystem* PlayerSubsystem = GetWorld()->GetSubsystem<UA302ServerPlayerSubsystem>())
    {
        PlayerSubsystem->HandlePlayerLogout(Exiting);
    }

	if (APlayerController* PC = Cast<APlayerController>(Exiting))
	{
		if (APlayerState* PS = PC->PlayerState)
		{
			FString PlayerName = PS->GetPlayerName();
			FString RoomCode = TEXT("");
			if (RoomMembershipRegistry)
			{
				RoomCode = RoomMembershipRegistry->GetPlayerRoomCode(PC);
			}   

			if (BackendRouter)
			{
				BackendRouter->NotifyPlayerLogout(PlayerName, RoomCode);
			}
		}
	}

    Super::Logout(Exiting);

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
    Super::HandleStartingNewPlayer_Implementation(NewPlayer);

    if (UA302ServerPlayerSubsystem* PlayerSubsystem = GetWorld()->GetSubsystem<UA302ServerPlayerSubsystem>())
    {
        PlayerSubsystem->HandlePlayerLogin(NewPlayer);
    }
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
		return false;
	}

    const bool bIsStandaloneLocal = A302GameplayGuards::IsStandaloneLocalExecution(InstigatorCharacter);
    if (!bIsStandaloneLocal)
    {
		if (const AA302PlayerState* InstigatorState = InstigatorCharacter->GetPlayerState<AA302PlayerState>())
		{
			if (!IsRoomGameplayActive(InstigatorState->GetRoomCode()))
			{
				UE_LOG(LogTemp, Warning, TEXT("[Reward] Personal event blocked: room gameplay inactive. player=%s room=%s reward=%s"), *GetNameSafe(InstigatorCharacter), *InstigatorState->GetRoomCode(), *GetNameSafe(RewardDefinition));
				return false;
			}
		}
	}

	return FA302ServerEventResolver::TryHandlePersonalEventReward(InstigatorCharacter, InteractedActor, RewardDefinition);
}

bool AA302GameMode::TryHandleGroupEventReward(ACharacter* InstigatorCharacter, AActor* InteractedActor, const URewardDefinition* RewardDefinition)
{
    if (!InstigatorCharacter || !A302GameplayGuards::IsGameplayEnabledCharacter(InstigatorCharacter))
    {
        return false;
    }

    const bool bIsStandaloneLocal = A302GameplayGuards::IsStandaloneLocalExecution(InstigatorCharacter);
    if (!bIsStandaloneLocal)
    {
        if (const AA302PlayerState* InstigatorState = InstigatorCharacter->GetPlayerState<AA302PlayerState>())
        {
            if (!IsRoomGameplayActive(InstigatorState->GetRoomCode()))
            {
                UE_LOG(LogTemp, Warning, TEXT("[Reward] Group event blocked: room gameplay inactive. player=%s room=%s reward=%s"), *GetNameSafe(InstigatorCharacter), *InstigatorState->GetRoomCode(), *GetNameSafe(RewardDefinition));
                return false;
            }
        }
    }

    return FA302ServerEventResolver::TryHandleGroupEventReward(InstigatorCharacter, InteractedActor, RewardDefinition);
}

void AA302GameMode::NotifyInteractionRewardResolved(
    ACharacter* InstigatorCharacter,
    const URewardDefinition* RewardDefinition,
    ERewardCategory EffectiveCategory
)
{
    if (!InstigatorCharacter || !RewardDefinition || !PhaseSubsystem)
    {
        return;
    }

    const AA302PlayerState* InstigatorState = InstigatorCharacter->GetPlayerState<AA302PlayerState>();
    if (!InstigatorState)
    {
        return;
    }

    const FString RoomCode = A302RoomScope::NormalizeRoomCode(InstigatorState->GetRoomCode());
    if (RoomCode.IsEmpty())
    {
        return;
    }

    PhaseSubsystem->NotifyRoomRewardResolved(RoomCode, EffectiveCategory);
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

    if (RoomRuntimeSubsystem)
    {
        RoomRuntimeSubsystem->StopRoomRuntime(NormalizedRoomCode);
    }

	if (BackendRouter)
	{
		BackendRouter->NotifyGameFinished(NormalizedRoomCode);
	}

	if (!RoomMembershipRegistry || !GetWorld())
	{
		return;
	}

	TArray<APlayerController*> RoomPlayers;
	RoomMembershipRegistry->GatherPlayersInRoom(GetWorld(), NormalizedRoomCode, RoomPlayers);

    const FString LobbyMapURL = TEXT("/Game/PersonalWorkSpace/sikk806/testLevel");

	for (APlayerController* RoomPlayer : RoomPlayers)
	{
		if (UA302ServerPlayerSubsystem* PlayerSubsystem = GetWorld()->GetSubsystem<UA302ServerPlayerSubsystem>())
		{
			PlayerSubsystem->UpdatePlayerGameplayFlag(RoomPlayer, false);
		}

        if (RoomPlayer)
        {
            if (AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(RoomPlayer))
            {
                const AMyCharacter* MyCharacter = Cast<AMyCharacter>(MyPlayerController->GetPawn());
                const AA302PlayerState* A302PlayerState = MyPlayerController->GetPlayerState<AA302PlayerState>();
                MyPlayerController->Client_ShowResultScreen(
                    BuildResultTitle(A302PlayerState, MyCharacter),
                    BuildResultDescription(A302PlayerState, MyCharacter),
                    ResultScreenDisplaySeconds
                );
            }

            UE_LOG(LogTemp, Log, TEXT("[A302GameMode] Returning player %s to lobby."), *RoomPlayer->GetName());
            FTimerHandle ReturnToLobbyTimerHandle;
            TWeakObjectPtr<APlayerController> WeakRoomPlayer(RoomPlayer);
            GetWorld()->GetTimerManager().SetTimer(
                ReturnToLobbyTimerHandle,
                FTimerDelegate::CreateLambda([WeakRoomPlayer, LobbyMapURL]()
                {
                    if (WeakRoomPlayer.IsValid())
                    {
                        WeakRoomPlayer->ClientTravel(LobbyMapURL, TRAVEL_Absolute);
                    }
                }),
                ResultScreenDisplaySeconds,
                false
            );
        }
	}
}

void AA302GameMode::HandleRoomLevelReady(const FString& RoomCode)
{
    SpawnPlayersInRoom(RoomCode);
}

bool AA302GameMode::IsRoomGameplayActive(const FString& RoomCode) const
{
    if (PhaseSubsystem)
    {
        return PhaseSubsystem->IsRoomPhaseActive(RoomCode);
    }
    return false;
}


void AA302GameMode::SpawnPlayersInRoom(const FString& RoomCode)
{
    const FString NormalizedRoomCode = A302RoomScope::NormalizeRoomCode(RoomCode);
    if (NormalizedRoomCode.IsEmpty() || !RoomMembershipRegistry)
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

    UA302ServerPlayerSubsystem* PlayerSubsystem = World->GetSubsystem<UA302ServerPlayerSubsystem>();
    if (!PlayerSubsystem) return;

	TArray<APlayerController*> RoomPlayers;
	RoomMembershipRegistry->GatherPlayersInRoom(World, NormalizedRoomCode, RoomPlayers);
	for (APlayerController* RoomPlayer : RoomPlayers)
	{
		PlayerSubsystem->UpdatePlayerGameplayFlag(RoomPlayer, true);
		PlayerSubsystem->QueueSpawnPlayer(RoomPlayer, true);
	}
}


void AA302GameMode::StartRoomGameplay(const FString& RoomCode)
{
    const FString NormalizedRoomCode = A302RoomScope::NormalizeRoomCode(RoomCode);
    if (NormalizedRoomCode.IsEmpty())
    {
        return;
    }

    if (PhaseSubsystem)
    {
        PhaseSubsystem->StartRoomPhaseTimeline(NormalizedRoomCode);
    }

    if (RoomRuntimeSubsystem)
    {
        RoomRuntimeSubsystem->PrepareRoom(NormalizedRoomCode);
    }
}


void AA302GameMode::OnMessageReceived(const FString &Message)
{
    if (BackendRouter)
    {
        BackendRouter->HandleMessage(Message);
    }
}
