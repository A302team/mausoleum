#include "Phase/A302ServerPhaseSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"
#include "TimerManager.h"

void UA302ServerPhaseSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UA302ServerPhaseSubsystem::HandleMapLoaded);
}

void UA302ServerPhaseSubsystem::Deinitialize()
{
    FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
    if (UWorld* World = ResolveWorld())
    {
        World->GetTimerManager().ClearTimer(PhaseUpdateTimerHandle);
    }

    bHasActiveRoom = false;
    ActiveRoomPhaseState = FA302RoomPhaseState();
    Super::Deinitialize();
}

bool UA302ServerPhaseSubsystem::StartRoomPhaseTimeline(const FString& RoomCode)
{
    if (RoomCode.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[A302ServerPhaseSubsystem] Cannot start room phase timeline with empty RoomCode."));
        return false;
    }

    if (bHasActiveRoom
        && !ActiveRoomPhaseState.RoomCode.IsEmpty()
        && ActiveRoomPhaseState.RoomCode != RoomCode
        && !ActiveRoomPhaseState.bFinished)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[A302ServerPhaseSubsystem] Rejecting room start. activeRoom=%s requestedRoom=%s"),
            *ActiveRoomPhaseState.RoomCode,
            *RoomCode
        );
        return false;
    }

    const double CurrentServerTime = FPlatformTime::Seconds();
    const bool bRestartingExistingRoom = bHasActiveRoom && ActiveRoomPhaseState.RoomCode == RoomCode;

    bHasActiveRoom = true;
    ActiveRoomPhaseState.RoomCode = RoomCode;
    ActiveRoomPhaseState.CurrentPhase = EGamePhase::Phase0;
    ActiveRoomPhaseState.MatchStartServerTime = static_cast<float>(CurrentServerTime);
    ActiveRoomPhaseState.PhaseChangedServerTime = static_cast<float>(CurrentServerTime);
    ActiveRoomPhaseState.bFinished = false;

    EnsurePhaseTimer();
    SyncGameStatePhase(ActiveRoomPhaseState);
    OnRoomPhaseChanged.Broadcast(RoomCode, ActiveRoomPhaseState.CurrentPhase);

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[A302ServerPhaseSubsystem] %s room phase timeline. room=%s start=%.2f"),
        bRestartingExistingRoom ? TEXT("Restarted") : TEXT("Started"),
        *RoomCode,
        CurrentServerTime
    );

    return true;
}

bool UA302ServerPhaseSubsystem::StopRoomPhaseTimeline(const FString& RoomCode)
{
    if (!bHasActiveRoom || RoomCode.IsEmpty())
    {
        return false;
    }

    if (ActiveRoomPhaseState.RoomCode != RoomCode)
    {
        return false;
    }

    if (UWorld* World = ResolveWorld())
    {
        World->GetTimerManager().ClearTimer(PhaseUpdateTimerHandle);
    }

    if (AA302GameState* GameState = ResolveGameState())
    {
        GameState->SetGamePhase(EGamePhase::Phase0, 0.0f);
    }

    bHasActiveRoom = false;
    ActiveRoomPhaseState = FA302RoomPhaseState();

    UE_LOG(LogTemp, Log, TEXT("[A302ServerPhaseSubsystem] Stopped room timeline. room=%s"), *RoomCode);
    return true;
}

bool UA302ServerPhaseSubsystem::TryGetRoomPhaseState(const FString& RoomCode, FA302RoomPhaseState& OutRoomState) const
{
    if (!bHasActiveRoom || ActiveRoomPhaseState.RoomCode != RoomCode)
    {
        return false;
    }

    OutRoomState = ActiveRoomPhaseState;
    return true;
}

void UA302ServerPhaseSubsystem::HandleMapLoaded(UWorld* LoadedWorld)
{
    if (!LoadedWorld || LoadedWorld->GetNetMode() == NM_Client)
    {
        return;
    }

    if (bHasActiveRoom)
    {
        EnsurePhaseTimer();
        SyncGameStatePhase(ActiveRoomPhaseState);
    }
}

void UA302ServerPhaseSubsystem::EvaluateRoomPhases()
{
    if (!bHasActiveRoom)
    {
        if (UWorld* World = ResolveWorld())
        {
            World->GetTimerManager().ClearTimer(PhaseUpdateTimerHandle);
        }
        return;
    }

    const double CurrentServerTime = FPlatformTime::Seconds();
    UpdateRoomPhase(CurrentServerTime);

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
        UE_LOG(LogTemp, Verbose, TEXT("[A302ServerPhaseSubsystem] Waiting for world before arming room timer."));
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
    }
}

void UA302ServerPhaseSubsystem::UpdateRoomPhase(double CurrentServerTime)
{
    if (!bHasActiveRoom || ActiveRoomPhaseState.bFinished)
    {
        return;
    }

    const double ElapsedSeconds = FMath::Max(0.0, CurrentServerTime - ActiveRoomPhaseState.MatchStartServerTime);
    const EGamePhase NewPhase = ResolvePhase(ElapsedSeconds);
    if (ActiveRoomPhaseState.CurrentPhase == NewPhase)
    {
        return;
    }

    ActiveRoomPhaseState.CurrentPhase = NewPhase;
    ActiveRoomPhaseState.PhaseChangedServerTime = static_cast<float>(CurrentServerTime);
    ActiveRoomPhaseState.bFinished = (NewPhase == EGamePhase::Ended);
    SyncGameStatePhase(ActiveRoomPhaseState);

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[A302ServerPhaseSubsystem] Room=%s phase changed to %s at %.2f (elapsed %.2f)"),
        *ActiveRoomPhaseState.RoomCode,
        ToString(NewPhase),
        CurrentServerTime,
        ElapsedSeconds
    );

    OnRoomPhaseChanged.Broadcast(ActiveRoomPhaseState.RoomCode, NewPhase);
}

void UA302ServerPhaseSubsystem::SyncGameStatePhase(const FA302RoomPhaseState& RoomState) const
{
    if (AA302GameState* GameState = ResolveGameState())
    {
        GameState->SetGamePhase(RoomState.CurrentPhase, RoomState.PhaseChangedServerTime);
    }
}

bool UA302ServerPhaseSubsystem::HasAnyActiveRoom() const
{
    return bHasActiveRoom && !ActiveRoomPhaseState.bFinished;
}

UWorld* UA302ServerPhaseSubsystem::ResolveWorld() const
{
    const UGameInstance* GameInstance = GetGameInstance();
    return GameInstance ? GameInstance->GetWorld() : nullptr;
}

AA302GameState* UA302ServerPhaseSubsystem::ResolveGameState() const
{
    UWorld* World = ResolveWorld();
    return World ? World->GetGameState<AA302GameState>() : nullptr;
}

EGamePhase UA302ServerPhaseSubsystem::ResolvePhase(double ElapsedSeconds) const
{
    const double Phase0End = FMath::Max(0.0f, Phase0Duration);
    const double Phase1End = Phase0End + FMath::Max(0.0f, Phase1Duration);
    const double Phase2End = Phase1End + FMath::Max(0.0f, Phase2Duration);

    if (ElapsedSeconds < Phase0End)
    {
        return EGamePhase::Phase0;
    }

    if (ElapsedSeconds < Phase1End)
    {
        return EGamePhase::Phase1;
    }

    if (ElapsedSeconds < Phase2End)
    {
        return EGamePhase::Phase2;
    }

    return EGamePhase::Ended;
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
