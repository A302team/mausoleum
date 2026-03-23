#include "Phase/A302ServerPhaseSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameData/Events/PersonalEvents/Interactable/PersonalEventPhase1CollectDefinition.h"
#include "GameData/RewardDefinition.h"
#include "GameData/RewardTypes.h"
#include "Character/MyPlayerController.h"
#include "GameMode/A302GameMode.h"
#include "GameMode/A302PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Room/RoomMembershipRegistry.h"
#include "Room/RoomWorldOffset.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogA302Phase, Log, All);

void UA302ServerPhaseSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UA302ServerPhaseSubsystem::HandleMapLoaded);
    UE_LOG(
        LogA302Phase,
        Log,
        TEXT("Initialized. RequiredCounts={P0:%d,P1:%d,P2:%d} PollInterval=%.2f PollingLog=%s"),
        Phase0RequiredItemCount,
        Phase1RequiredClearObjectCount,
        Phase2RequiredGroupEventCount,
        PhasePollInterval,
        bLogPhasePolling ? TEXT("true") : TEXT("false")
    );
}

void UA302ServerPhaseSubsystem::Deinitialize()
{
    FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
    if (UWorld* World = ResolveWorld())
    {
        World->GetTimerManager().ClearTimer(PhaseUpdateTimerHandle);
    }

    RoomPhaseStates.Reset();
    Super::Deinitialize();
}

bool UA302ServerPhaseSubsystem::StartRoomPhaseTimeline(const FString& RoomCode)
{
    const FString NormalizedRoomCode = A302RoomWorldOffset::NormalizeRoomCode(RoomCode);
    if (NormalizedRoomCode.IsEmpty())
    {
        UE_LOG(LogA302Phase, Warning, TEXT("Cannot start room phase timeline with empty RoomCode."));
        return false;
    }

    const double CurrentServerTime = FPlatformTime::Seconds();

    FA302RoomPhaseState& RoomState = RoomPhaseStates.FindOrAdd(NormalizedRoomCode);
    const bool bRestartingExistingRoom = !RoomState.RoomCode.IsEmpty() && !RoomState.bFinished;

    RoomState.RoomCode = NormalizedRoomCode;
    RoomState.CurrentPhase = EGamePhase::Phase0;
    RoomState.MatchStartServerTime = static_cast<float>(CurrentServerTime);
    RoomState.PhaseChangedServerTime = static_cast<float>(CurrentServerTime);
    RoomState.Phase0ItemCount = 0;
    RoomState.Phase1ClearObjectCount = 0;
    RoomState.Phase2GroupEventCount = 0;
    RoomState.bTimedOutWithoutEscape = false;
    RoomState.bFinished = false;

    EnsurePhaseTimer();
    BroadcastMatchTimerStateToRoom(NormalizedRoomCode, RoomState.MatchStartServerTime, MatchTimeLimitSeconds, MatchTimeLimitSeconds > 0.0f);
    OnRoomPhaseChanged.Broadcast(NormalizedRoomCode, RoomState.CurrentPhase);

    UE_LOG(
        LogA302Phase,
        Log,
        TEXT("%s room phase timeline. room=%s start=%.2f progress=%s"),
        bRestartingExistingRoom ? TEXT("Restarted") : TEXT("Started"),
        *NormalizedRoomCode,
        CurrentServerTime,
        *BuildRoomProgressSummary(RoomState)
    );

    return true;
}

bool UA302ServerPhaseSubsystem::StopRoomPhaseTimeline(const FString& RoomCode)
{
    const FString NormalizedRoomCode = A302RoomWorldOffset::NormalizeRoomCode(RoomCode);
    if (NormalizedRoomCode.IsEmpty())
    {
        return false;
    }

    const bool bRemoved = RoomPhaseStates.Remove(NormalizedRoomCode) > 0;
    if (!bRemoved)
    {
        UE_LOG(LogA302Phase, Verbose, TEXT("Stop ignored. room=%s was not active."), *NormalizedRoomCode);
        return false;
    }

    if (!HasAnyActiveRoom())
    {
        if (UWorld* World = ResolveWorld())
        {
            World->GetTimerManager().ClearTimer(PhaseUpdateTimerHandle);
        }
    }

    UE_LOG(LogA302Phase, Log, TEXT("Stopped room timeline. room=%s"), *NormalizedRoomCode);
    return true;
}

bool UA302ServerPhaseSubsystem::TryGetRoomPhaseState(const FString& RoomCode, FA302RoomPhaseState& OutRoomState) const
{
    const FString NormalizedRoomCode = A302RoomWorldOffset::NormalizeRoomCode(RoomCode);
    if (const FA302RoomPhaseState* RoomState = RoomPhaseStates.Find(NormalizedRoomCode))
    {
        OutRoomState = *RoomState;
        return true;
    }

    return false;
}

bool UA302ServerPhaseSubsystem::IsRoomPhaseActive(const FString& RoomCode) const
{
    const FString NormalizedRoomCode = A302RoomWorldOffset::NormalizeRoomCode(RoomCode);
    if (const FA302RoomPhaseState* RoomState = RoomPhaseStates.Find(NormalizedRoomCode))
    {
        return !RoomState->bFinished;
    }

    return false;
}

void UA302ServerPhaseSubsystem::NotifyRoomRewardResolved(const FString& RoomCode, const URewardDefinition* RewardDefinition, ERewardCategory RewardCategory)
{
    const FString NormalizedRoomCode = A302RoomWorldOffset::NormalizeRoomCode(RoomCode);
    FA302RoomPhaseState* RoomState = RoomPhaseStates.Find(NormalizedRoomCode);
    if (!RoomState || RoomState->bFinished)
    {
        UE_LOG(
            LogA302Phase,
            Verbose,
            TEXT("Reward ignored. room=%s state=%s"),
            *NormalizedRoomCode,
            RoomState ? TEXT("finished") : TEXT("missing")
        );
        return;
    }

    bool bCounted = false;
    switch (RoomState->CurrentPhase)
    {
    case EGamePhase::Phase0:
        if (RewardCategory == ERewardCategory::BasicItem)
        {
            ++RoomState->Phase0ItemCount;
            bCounted = true;
        }
        break;
    case EGamePhase::Phase1:
        if (Cast<UPersonalEventPhase1CollectDefinition>(const_cast<URewardDefinition*>(RewardDefinition)) != nullptr)
        {
            ++RoomState->Phase1ClearObjectCount;
            bCounted = true;
        }
        break;
    case EGamePhase::Phase2:
        if (RewardCategory == ERewardCategory::GroupEvent)
        {
            ++RoomState->Phase2GroupEventCount;
            bCounted = true;
        }
        break;
    default:
        break;
    }

    if (bCounted)
    {
        UE_LOG(
            LogA302Phase,
            Log,
            TEXT("Reward counted. room=%s phase=%s category=%s progress=%s"),
            *NormalizedRoomCode,
            ToString(RoomState->CurrentPhase),
            ToString(RewardCategory),
            *BuildRoomProgressSummary(*RoomState)
        );
    }
    else
    {
        UE_LOG(
            LogA302Phase,
            Verbose,
            TEXT("Reward skipped by phase rule. room=%s phase=%s category=%s progress=%s"),
            *NormalizedRoomCode,
            ToString(RoomState->CurrentPhase),
            ToString(RewardCategory),
            *BuildRoomProgressSummary(*RoomState)
        );
    }
}

void UA302ServerPhaseSubsystem::HandleMapLoaded(UWorld* LoadedWorld)
{
    if (!LoadedWorld || LoadedWorld->GetNetMode() == NM_Client)
    {
        return;
    }

    if (HasAnyActiveRoom())
    {
        EnsurePhaseTimer();
    }
}

void UA302ServerPhaseSubsystem::EvaluateRoomPhases()
{
    if (RoomPhaseStates.Num() == 0)
    {
        if (UWorld* World = ResolveWorld())
        {
            World->GetTimerManager().ClearTimer(PhaseUpdateTimerHandle);
        }
        return;
    }

    const double CurrentServerTime = FPlatformTime::Seconds();

    TArray<FString> RoomCodes;
    RoomPhaseStates.GetKeys(RoomCodes);
    for (const FString& RoomCode : RoomCodes)
    {
        if (bLogPhasePolling)
        {
            if (const FA302RoomPhaseState* RoomState = RoomPhaseStates.Find(RoomCode))
            {
                UE_LOG(LogA302Phase, Verbose, TEXT("Polling room=%s progress=%s"), *RoomCode, *BuildRoomProgressSummary(*RoomState));
            }
        }
        UpdateRoomPhase(RoomCode, CurrentServerTime);
    }

    if (!HasAnyActiveRoom())
    {
        if (UWorld* World = ResolveWorld())
        {
            World->GetTimerManager().ClearTimer(PhaseUpdateTimerHandle);
        }
    }
}

void UA302ServerPhaseSubsystem::EnsurePhaseTimer()
{
    UWorld* World = ResolveWorld();
    if (!World)
    {
        UE_LOG(LogA302Phase, Verbose, TEXT("Waiting for world before arming room timer."));
        return;
    }

    const float SafePollInterval = FMath::Max(0.05f, PhasePollInterval);
    if (!World->GetTimerManager().IsTimerActive(PhaseUpdateTimerHandle))
    {
        World->GetTimerManager().SetTimer(
            PhaseUpdateTimerHandle,
            this,
            &UA302ServerPhaseSubsystem::EvaluateRoomPhases,
            SafePollInterval,
            true
        );
        UE_LOG(LogA302Phase, Log, TEXT("Phase timer armed. interval=%.2f"), SafePollInterval);
    }
}

void UA302ServerPhaseSubsystem::UpdateRoomPhase(const FString& RoomCode, double CurrentServerTime)
{
    const FString NormalizedRoomCode = A302RoomWorldOffset::NormalizeRoomCode(RoomCode);
    FA302RoomPhaseState* RoomState = RoomPhaseStates.Find(NormalizedRoomCode);
    if (!RoomState || RoomState->bFinished)
    {
        return;
    }

    const EGamePhase PreviousPhase = RoomState->CurrentPhase;
    const bool bTimedOutWithoutEscape =
        MatchTimeLimitSeconds > 0.0f &&
        (CurrentServerTime - RoomState->MatchStartServerTime) >= MatchTimeLimitSeconds &&
        CountEscapedPlayersInRoom(RoomState->RoomCode) <= 0;
    const EGamePhase NewPhase = ResolvePhase(*RoomState);
    if (PreviousPhase == NewPhase)
    {
        return;
    }

    FString TransitionReason = TEXT("rule satisfied");
    if (PreviousPhase == EGamePhase::Phase0 && NewPhase == EGamePhase::Phase1)
    {
        TransitionReason = FString::Printf(
            TEXT("Phase0 item count reached (%d/%d)"),
            RoomState->Phase0ItemCount,
            FMath::Max(0, Phase0RequiredItemCount)
        );
    }
    else if (PreviousPhase == EGamePhase::Phase1 && NewPhase == EGamePhase::Phase2)
    {
        TransitionReason = FString::Printf(
            TEXT("Phase1 clear object count reached (%d/%d)"),
            RoomState->Phase1ClearObjectCount,
            FMath::Max(0, Phase1RequiredClearObjectCount)
        );
    }
    else if (PreviousPhase == EGamePhase::Phase2 && NewPhase == EGamePhase::Ended)
    {
        if (bTimedOutWithoutEscape)
        {
            TransitionReason = FString::Printf(
                TEXT("No player escaped within time limit (elapsed=%.0fs, limit=%.0fs)"),
                CurrentServerTime - RoomState->MatchStartServerTime,
                MatchTimeLimitSeconds
            );
        }
        else
        {
            const int32 RequiredGroupEvents = FMath::Max(0, Phase2RequiredGroupEventCount);
            const int32 AliveCount = CountAlivePlayersInRoom(RoomState->RoomCode);
            if (RequiredGroupEvents > 0 && RoomState->Phase2GroupEventCount >= RequiredGroupEvents)
            {
                TransitionReason = FString::Printf(
                    TEXT("Phase2 group event count reached (%d/%d)"),
                    RoomState->Phase2GroupEventCount,
                    RequiredGroupEvents
                );
            }
            else
            {
                TransitionReason = FString::Printf(TEXT("Last survivor condition met (alive=%d)"), AliveCount);
            }
        }
    }

    RoomState->CurrentPhase = NewPhase;
    RoomState->PhaseChangedServerTime = static_cast<float>(CurrentServerTime);
    RoomState->bTimedOutWithoutEscape = bTimedOutWithoutEscape && NewPhase == EGamePhase::Ended;
    RoomState->bFinished = (NewPhase == EGamePhase::Ended);
    if (RoomState->bFinished)
    {
        BroadcastMatchTimerStateToRoom(NormalizedRoomCode, RoomState->MatchStartServerTime, MatchTimeLimitSeconds, false);
    }

    UE_LOG(
        LogA302Phase,
        Log,
        TEXT("Room=%s phase changed %s -> %s at %.2f reason=%s progress=%s"),
        *NormalizedRoomCode,
        ToString(PreviousPhase),
        ToString(NewPhase),
        CurrentServerTime,
        *TransitionReason,
        *BuildRoomProgressSummary(*RoomState)
    );

    OnRoomPhaseChanged.Broadcast(NormalizedRoomCode, NewPhase);
}

bool UA302ServerPhaseSubsystem::HasAnyActiveRoom() const
{
    for (const TPair<FString, FA302RoomPhaseState>& Pair : RoomPhaseStates)
    {
        if (!Pair.Value.bFinished)
        {
            return true;
        }
    }

    return false;
}

UWorld* UA302ServerPhaseSubsystem::ResolveWorld() const
{
    const UGameInstance* GameInstance = GetGameInstance();
    return GameInstance ? GameInstance->GetWorld() : nullptr;
}

EGamePhase UA302ServerPhaseSubsystem::ResolvePhase(const FA302RoomPhaseState& RoomState) const
{
    if (MatchTimeLimitSeconds > 0.0f)
    {
        const double CurrentServerTime = FPlatformTime::Seconds();
        const double ElapsedSeconds = CurrentServerTime - RoomState.MatchStartServerTime;
        if (ElapsedSeconds >= MatchTimeLimitSeconds && CountEscapedPlayersInRoom(RoomState.RoomCode) <= 0)
        {
            return EGamePhase::Ended;
        }
    }

    switch (RoomState.CurrentPhase)
    {
    case EGamePhase::Phase0:
    {
        const int32 RequiredItemCount = FMath::Max(0, Phase0RequiredItemCount);
        if (RequiredItemCount <= 0 || RoomState.Phase0ItemCount >= RequiredItemCount)
        {
            return EGamePhase::Phase1;
        }
        return EGamePhase::Phase0;
    }
    case EGamePhase::Phase1:
    {
        const int32 RequiredClearCount = FMath::Max(0, Phase1RequiredClearObjectCount);
        if (RequiredClearCount <= 0 || RoomState.Phase1ClearObjectCount >= RequiredClearCount)
        {
            return EGamePhase::Phase2;
        }
        return EGamePhase::Phase1;
    }
    case EGamePhase::Phase2:
    {
        const int32 RequiredGroupEvents = FMath::Max(0, Phase2RequiredGroupEventCount);
        const bool bGroupEventsCleared = RequiredGroupEvents > 0 && RoomState.Phase2GroupEventCount >= RequiredGroupEvents;
        const bool bLastSurvivor = CountAlivePlayersInRoom(RoomState.RoomCode) <= 1;
        if (bGroupEventsCleared || bLastSurvivor)
        {
            return EGamePhase::Ended;
        }
        return EGamePhase::Phase2;
    }
    case EGamePhase::Ended:
    default:
        return RoomState.CurrentPhase;
    }
}

int32 UA302ServerPhaseSubsystem::CountAlivePlayersInRoom(const FString& RoomCode) const
{
    UWorld* World = ResolveWorld();
    if (!World)
    {
        return 0;
    }

    const AA302GameMode* GameMode = Cast<AA302GameMode>(World->GetAuthGameMode());
    if (!GameMode)
    {
        return 0;
    }

    URoomMembershipRegistry* Registry = GameMode->GetRoomMembershipRegistry();
    if (!Registry)
    {
        return 0;
    }

    TArray<APlayerController*> Players;
    Registry->GatherPlayersInRoom(World, RoomCode, Players);

    int32 AliveCount = 0;
    for (APlayerController* PlayerController : Players)
    {
        const AA302PlayerState* PlayerState = PlayerController ? PlayerController->GetPlayerState<AA302PlayerState>() : nullptr;
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

int32 UA302ServerPhaseSubsystem::CountEscapedPlayersInRoom(const FString& RoomCode) const
{
    UWorld* World = ResolveWorld();
    if (!World)
    {
        return 0;
    }

    const AA302GameMode* GameMode = Cast<AA302GameMode>(World->GetAuthGameMode());
    if (!GameMode)
    {
        return 0;
    }

    URoomMembershipRegistry* Registry = GameMode->GetRoomMembershipRegistry();
    if (!Registry)
    {
        return 0;
    }

    TArray<APlayerController*> Players;
    Registry->GatherPlayersInRoom(World, RoomCode, Players);

    int32 EscapedCount = 0;
    for (APlayerController* PlayerController : Players)
    {
        const AA302PlayerState* PlayerState = PlayerController ? PlayerController->GetPlayerState<AA302PlayerState>() : nullptr;
        if (PlayerState && PlayerState->bIsEscaped)
        {
            ++EscapedCount;
        }
    }

    return EscapedCount;
}

void UA302ServerPhaseSubsystem::BroadcastMatchTimerStateToRoom(const FString& RoomCode, float MatchStartServerTime, float DurationSeconds, bool bVisible) const
{
    UWorld* World = ResolveWorld();
    if (!World)
    {
        return;
    }

    const AA302GameMode* GameMode = Cast<AA302GameMode>(World->GetAuthGameMode());
    if (!GameMode)
    {
        return;
    }

    URoomMembershipRegistry* Registry = GameMode->GetRoomMembershipRegistry();
    if (!Registry)
    {
        return;
    }

    TArray<APlayerController*> Players;
    Registry->GatherPlayersInRoom(World, RoomCode, Players);

    for (APlayerController* PlayerController : Players)
    {
        if (AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(PlayerController))
        {
            MyPlayerController->ConfigureMatchTimer(MatchStartServerTime, DurationSeconds, bVisible);
        }
    }
}

FString UA302ServerPhaseSubsystem::BuildRoomProgressSummary(const FA302RoomPhaseState& RoomState) const
{
    return FString::Printf(
        TEXT("phase=%s p0=%d/%d p1=%d/%d p2=%d/%d alive=%d escaped=%d timeout=%s"),
        ToString(RoomState.CurrentPhase),
        RoomState.Phase0ItemCount,
        FMath::Max(0, Phase0RequiredItemCount),
        RoomState.Phase1ClearObjectCount,
        FMath::Max(0, Phase1RequiredClearObjectCount),
        RoomState.Phase2GroupEventCount,
        FMath::Max(0, Phase2RequiredGroupEventCount),
        CountAlivePlayersInRoom(RoomState.RoomCode),
        CountEscapedPlayersInRoom(RoomState.RoomCode),
        RoomState.bTimedOutWithoutEscape ? TEXT("true") : TEXT("false")
    );
}

const TCHAR* UA302ServerPhaseSubsystem::ToString(EGamePhase Phase)
{
    switch (Phase)
    {
    case EGamePhase::Phase0:
        return TEXT("Phase0");
    case EGamePhase::Phase1:
        return TEXT("Phase1");
    case EGamePhase::Phase2:
        return TEXT("Phase2");
    case EGamePhase::Ended:
        return TEXT("Ended");
    default:
        return TEXT("Unknown");
    }
}

const TCHAR* UA302ServerPhaseSubsystem::ToString(ERewardCategory RewardCategory)
{
    switch (RewardCategory)
    {
    case ERewardCategory::BasicItem:
        return TEXT("BasicItem");
    case ERewardCategory::PersonalEvent:
        return TEXT("PersonalEvent");
    case ERewardCategory::GroupEvent:
        return TEXT("GroupEvent");
    default:
        return TEXT("UnknownReward");
    }
}
