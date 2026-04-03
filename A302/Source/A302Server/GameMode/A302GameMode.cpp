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
#include "Subsystem/A302ItemSpawnSubsystem.h"
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
#include "TimerManager.h"

namespace
{
    constexpr float ResultScreenDisplaySeconds = 5.0f;
    constexpr int32 EvilMaliceThreshold = 3;

    enum class EResultFaction : uint8
    {
        Innocent,
        Evil
    };

    enum class EResultOutcome : uint8
    {
        InnocentVictory,
        EvilVictory,
        AllDefeat
    };

    struct FRoomResultSummary
    {
        EResultOutcome Outcome = EResultOutcome::EvilVictory;
        bool bAnyEscapedInnocent = false;
        bool bAnyEscapedEvil = false;
        bool bTimedOutWithoutEscape = false;
    };

    bool IsEvilByMaliceCount(int32 MaliceCount)
    {
        return MaliceCount >= EvilMaliceThreshold;
    }

    int32 ResolvePlayerMaliceCount(const AMyCharacter* Character)
    {
        const UMaliceComponent* MaliceComponent = Character ? Character->FindComponentByClass<UMaliceComponent>() : nullptr;
        return MaliceComponent ? MaliceComponent->GetMaliceCount() : 0;
    }

    EResultFaction ResolvePlayerFaction(const AA302PlayerState* PlayerState, const AMyCharacter* Character)
    {
        if (Character)
        {
            return IsEvilByMaliceCount(ResolvePlayerMaliceCount(Character))
                ? EResultFaction::Evil
                : EResultFaction::Innocent;
        }

        if (PlayerState)
        {
            return PlayerState->PlayerRole == EPlayerRole::Evil
                ? EResultFaction::Evil
                : EResultFaction::Innocent;
        }

        return EResultFaction::Innocent;
    }

    FRoomResultSummary BuildRoomResultSummary(const TArray<APlayerController*>& RoomPlayers, bool bTimedOutWithoutEscape)
    {
        FRoomResultSummary Summary;
        Summary.bTimedOutWithoutEscape = bTimedOutWithoutEscape;

        for (APlayerController* RoomPlayer : RoomPlayers)
        {
            const AA302PlayerState* PlayerState = RoomPlayer ? RoomPlayer->GetPlayerState<AA302PlayerState>() : nullptr;
            const AMyCharacter* MyCharacter = RoomPlayer ? Cast<AMyCharacter>(RoomPlayer->GetPawn()) : nullptr;
            if (!PlayerState || !PlayerState->bIsEscaped)
            {
                continue;
            }

            const EResultFaction PlayerFaction = ResolvePlayerFaction(PlayerState, MyCharacter);
            if (PlayerFaction == EResultFaction::Innocent)
            {
                Summary.bAnyEscapedInnocent = true;
            }
            else
            {
                Summary.bAnyEscapedEvil = true;
            }
        }

        if (Summary.bTimedOutWithoutEscape)
        {
            Summary.Outcome = EResultOutcome::AllDefeat;
        }
        else if (Summary.bAnyEscapedInnocent)
        {
            Summary.Outcome = EResultOutcome::InnocentVictory;
        }
        else
        {
            Summary.Outcome = EResultOutcome::EvilVictory;
        }

        return Summary;
    }

    FText BuildFactionText(EResultFaction Faction)
    {
        return Faction == EResultFaction::Evil
            ? NSLOCTEXT("A302GameMode", "RoleEvil", "악인")
            : NSLOCTEXT("A302GameMode", "RoleInnocent", "선인");
    }

    FText BuildResultRoleText(const AA302PlayerState* PlayerState, const AMyCharacter* Character)
    {
        return BuildFactionText(ResolvePlayerFaction(PlayerState, Character));
    }

    FText BuildResultStatusText(const AA302PlayerState* PlayerState)
    {
        return PlayerState && PlayerState->bIsEscaped
            ? NSLOCTEXT("A302GameMode", "StatusEscaped", "탈출함")
            : (PlayerState && !PlayerState->bIsAlive
                ? NSLOCTEXT("A302GameMode", "StatusDead", "사망함")
                : NSLOCTEXT("A302GameMode", "StatusSurvived", "생존"));
    }

    FText BuildOutcomeNarration(EResultFaction WinningFaction)
    {
        return WinningFaction == EResultFaction::Innocent
            ? NSLOCTEXT("A302GameMode", "OutcomeNarrationInnocent", "끝내 한 줄기 빛이 문턱을 넘어섰습니다. 선인 진영이 승리를 거머쥐었습니다.")
            : NSLOCTEXT("A302GameMode", "OutcomeNarrationEvil", "끝내 빛은 문을 넘지 못했고, 남겨진 어둠이 이 밤의 결말이 되었습니다. 악인 진영이 승리했습니다.");
    }

    FText BuildPersonalNarration(const AA302PlayerState* PlayerState)
    {
        if (PlayerState && PlayerState->bIsEscaped)
        {
            return NSLOCTEXT("A302GameMode", "PersonalNarrationEscaped", "당신은 마침내 출구를 지나, 이곳을 빠져나오는 데 성공했습니다.");
        }

        if (PlayerState && !PlayerState->bIsAlive)
        {
            return NSLOCTEXT("A302GameMode", "PersonalNarrationDead", "당신은 끝내 이 밤을 벗어나지 못한 채, 그 자리에 멈추고 말았습니다.");
        }

        return NSLOCTEXT("A302GameMode", "PersonalNarrationSurvived", "당신은 끝까지 버텨냈지만, 스스로 문을 넘지는 못했습니다.");
    }

    FText BuildResultTitle(const AA302PlayerState* PlayerState, const AMyCharacter* Character, const FRoomResultSummary& Summary)
    {
        if (Summary.Outcome == EResultOutcome::AllDefeat)
        {
            return NSLOCTEXT("A302GameMode", "AllDefeatTitle", "끝내 아무도 문을 넘지 못했습니다");
        }

        const EResultFaction PlayerFaction = ResolvePlayerFaction(PlayerState, Character);
        const bool bPlayerWon =
            (Summary.Outcome == EResultOutcome::InnocentVictory && PlayerFaction == EResultFaction::Innocent) ||
            (Summary.Outcome == EResultOutcome::EvilVictory && PlayerFaction == EResultFaction::Evil);
        if (bPlayerWon)
        {
            return PlayerState && PlayerState->bIsEscaped
                ? NSLOCTEXT("A302GameMode", "VictoryEscapedTitle", "마침내 문을 넘었습니다")
                : NSLOCTEXT("A302GameMode", "VictoryTitle", "당신은 살아남았습니다");
        }

        return PlayerState && PlayerState->bIsEscaped
            ? NSLOCTEXT("A302GameMode", "DefeatEscapedTitle", "문을 넘었지만 끝내 패배했습니다")
            : NSLOCTEXT("A302GameMode", "DefeatTitle", "이 밤은 당신의 편이 아니었습니다");
    }

    FText BuildResultDescription(const AA302PlayerState* PlayerState, const AMyCharacter* Character, const FRoomResultSummary& Summary)
    {
        const int32 MaliceCount = ResolvePlayerMaliceCount(Character);
        const FText RoleText = BuildResultRoleText(PlayerState, Character);
        const FText StatusText = BuildResultStatusText(PlayerState);

        if (Summary.Outcome == EResultOutcome::AllDefeat)
        {
            return FText::Format(
                NSLOCTEXT("A302GameMode", "AllDefeatDescription", "끝내 정해진 시간 안에 누구도 문을 넘지 못했습니다.\n이 밤은 누구에게도 출구를 허락하지 않았습니다.\n\n당신의 역할은 {0}, 최종 상태는 {1}입니다.\n당신 안에 남은 악의는 {2}입니다."),
                RoleText,
                StatusText,
                FText::AsNumber(MaliceCount)
            );
        }

        return FText::Format(
            NSLOCTEXT("A302GameMode", "ResultDescription", "{0}\n\n{1}\n\n당신의 역할은 {2}, 최종 상태는 {3}입니다.\n당신 안에 남은 악의는 {4}입니다."),
            BuildOutcomeNarration(Summary.Outcome == EResultOutcome::InnocentVictory ? EResultFaction::Innocent : EResultFaction::Evil),
            BuildPersonalNarration(PlayerState),
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
            ItemSpawnSubsystem = GameInstance->GetSubsystem<UA302ItemSpawnSubsystem>();
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
    // HandleStartingNewPlayer_Implementation에서 HandlePlayerLogin을 호출하므로 여기선 중복 호출하지 않음
}

void AA302GameMode::Logout(AController *Exiting)
{
    FString LeavingRoomCode;

    if (UA302ServerPlayerSubsystem* PlayerSubsystem = GetWorld()->GetSubsystem<UA302ServerPlayerSubsystem>())
    {
        PlayerSubsystem->HandlePlayerLogout(Exiting);
    }

	if (APlayerController* PC = Cast<APlayerController>(Exiting))
	{
		if (APlayerState* PS = PC->PlayerState)
		{
			FString PlayerName = PS->GetPlayerName();
			if (RoomMembershipRegistry)
			{
				LeavingRoomCode = RoomMembershipRegistry->GetPlayerRoomCode(PC);
			}   

			if (BackendRouter)
			{
				BackendRouter->NotifyPlayerLogout(PlayerName, LeavingRoomCode);
			}
		}
	}

    Super::Logout(Exiting);
    SyncRoomStateToGameState(LeavingRoomCode);

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
    // GroupEvent(석상 등)는 RewardDefinition 없이 페이즈 카운터만 올리면 됨
    if (!InstigatorCharacter || !PhaseSubsystem)
    {
        return;
    }
    if (!RewardDefinition && EffectiveCategory != ERewardCategory::GroupEvent)
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

    PhaseSubsystem->NotifyRoomRewardResolved(RoomCode, RewardDefinition, EffectiveCategory);
}


void AA302GameMode::HandleRoomPhaseChanged(const FString& RoomCode, EGamePhase NewPhase)
{
	const FString NormalizedRoomCode = A302RoomScope::NormalizeRoomCode(RoomCode);
	if (NormalizedRoomCode.IsEmpty())
	{
		return;
	}

    SyncRoomStateToGameState(NormalizedRoomCode);

    if (ItemSpawnSubsystem)
    {
        ItemSpawnSubsystem->HandleRoomPhaseChanged(NormalizedRoomCode, NewPhase);
    }

    // Phase1, Phase2 전환 시 해당 구역 스폰 포인트로 텔레포트
    if (NewPhase == EGamePhase::Phase1 || NewPhase == EGamePhase::Phase2)
    {
        if (RoomMembershipRegistry && GetWorld() && EnsureSpawnManager())
        {
            TArray<APlayerController*> RoomPlayers;
            RoomMembershipRegistry->GatherPlayersInRoom(GetWorld(), NormalizedRoomCode, RoomPlayers);
            SpawnManager->TeleportPlayersToPhaseSpawnPoints(RoomPlayers, NormalizedRoomCode, NewPhase);
        }
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
    bool bTimedOutWithoutEscape = false;
    if (PhaseSubsystem)
    {
        FA302RoomPhaseState RoomPhaseState;
        if (PhaseSubsystem->TryGetRoomPhaseState(NormalizedRoomCode, RoomPhaseState))
        {
            bTimedOutWithoutEscape = RoomPhaseState.bTimedOutWithoutEscape;
        }
    }
    const FRoomResultSummary RoomResultSummary = BuildRoomResultSummary(RoomPlayers, bTimedOutWithoutEscape);

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
                    BuildResultTitle(A302PlayerState, MyCharacter, RoomResultSummary),
                    BuildResultDescription(A302PlayerState, MyCharacter, RoomResultSummary),
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
    const FString NormalizedRoomCode = A302RoomScope::NormalizeRoomCode(RoomCode);
    if (NormalizedRoomCode.IsEmpty())
    {
        return;
    }

    SpawnPlayersInRoom(NormalizedRoomCode);

    EGamePhase CurrentPhase = EGamePhase::Phase0;
    if (PhaseSubsystem)
    {
        FA302RoomPhaseState RoomPhaseState;
        if (PhaseSubsystem->TryGetRoomPhaseState(NormalizedRoomCode, RoomPhaseState))
        {
            CurrentPhase = RoomPhaseState.CurrentPhase;
        }
    }

    if (ItemSpawnSubsystem)
    {
        ItemSpawnSubsystem->HandleRoomLevelReady(NormalizedRoomCode, CurrentPhase);
    }
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

    SyncRoomStateToGameState(NormalizedRoomCode);

    // 캐릭터 스폰 완료를 대기하기 위해 짧은 딜레이 후 타이머 시작
    if (PhaseSubsystem)
    {
        FTimerHandle TimerHandle;
        TWeakObjectPtr<UA302ServerPhaseSubsystem> WeakPhaseSubsystem(PhaseSubsystem);
        World->GetTimerManager().SetTimer(
            TimerHandle,
            FTimerDelegate::CreateLambda([WeakPhaseSubsystem, NormalizedRoomCode]()
            {
                if (WeakPhaseSubsystem.IsValid())
                {
                    WeakPhaseSubsystem->NotifyRoomMatchTimerStart(NormalizedRoomCode);
                }
            }),
            1.5f, // 캐릭터 스폰 Warmup 시간 고려
            false
        );
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

    SyncRoomStateToGameState(NormalizedRoomCode);
}

void AA302GameMode::SyncRoomStateToGameState(const FString& RoomCode)
{
    const FString NormalizedRoomCode = A302RoomScope::NormalizeRoomCode(RoomCode);
    if (NormalizedRoomCode.IsEmpty())
    {
        return;
    }

    AA302GameState* A302GameState = GetGameState<AA302GameState>();
    if (!A302GameState)
    {
        return;
    }

    if (PhaseSubsystem)
    {
        FA302RoomPhaseState RoomPhaseState;
        if (PhaseSubsystem->TryGetRoomPhaseState(NormalizedRoomCode, RoomPhaseState))
        {
            A302GameState->SetGamePhase(RoomPhaseState.CurrentPhase, RoomPhaseState.PhaseChangedServerTime, NormalizedRoomCode);

            int32 CurrentCount = 0;
            int32 RequiredCount = 0;
            switch (RoomPhaseState.CurrentPhase)
            {
            case EGamePhase::Phase0:
                CurrentCount = RoomPhaseState.Phase0ItemCount;
                RequiredCount = PhaseSubsystem->Phase0RequiredItemCount;
                break;
            case EGamePhase::Phase1:
                CurrentCount = RoomPhaseState.Phase1ClearObjectCount;
                RequiredCount = PhaseSubsystem->Phase1RequiredClearObjectCount;
                break;
            case EGamePhase::Phase2:
                CurrentCount = RoomPhaseState.Phase2GroupEventCount;
                RequiredCount = PhaseSubsystem->Phase2RequiredGroupEventCount;
                break;
            default:
                break;
            }

            if (RoomMembershipRegistry && GetWorld())
            {
                TArray<APlayerController*> RoomPlayers;
                RoomMembershipRegistry->GatherPlayersInRoom(GetWorld(), NormalizedRoomCode, RoomPlayers);

                const bool bShowProgressUI = RoomPhaseState.CurrentPhase != EGamePhase::Ended;
                for (APlayerController* RoomPlayer : RoomPlayers)
                {
                    if (AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(RoomPlayer))
                    {
                        MyPlayerController->UpdatePhaseClearProgress(
                            static_cast<uint8>(RoomPhaseState.CurrentPhase),
                            FMath::Max(0, CurrentCount),
                            FMath::Max(0, RequiredCount),
                            bShowProgressUI
                        );
                    }
                }
            }
        }
    }
}

int32 AA302GameMode::CountAlivePlayersInRoom(const FString& RoomCode) const
{
    if (!RoomMembershipRegistry || !GetWorld())
    {
        return 0;
    }

    TArray<APlayerController*> RoomPlayers;
    RoomMembershipRegistry->GatherPlayersInRoom(GetWorld(), RoomCode, RoomPlayers);

    int32 AliveCount = 0;
    for (APlayerController* RoomPlayer : RoomPlayers)
    {
        const AA302PlayerState* PlayerState = RoomPlayer ? RoomPlayer->GetPlayerState<AA302PlayerState>() : nullptr;
        if (!PlayerState)
        {
            continue;
        }

        if (PlayerState->bIsAlive && !PlayerState->bIsEscaped)
        {
            ++AliveCount;
        }
    }

    return AliveCount;
}


void AA302GameMode::OnMessageReceived(const FString &Message)
{
    if (BackendRouter)
    {
        BackendRouter->HandleMessage(Message);
    }
}
