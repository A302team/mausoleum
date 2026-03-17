#include "Phase/A302ServerPhaseSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Room/RoomWorldOffset.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

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

    RoomPhaseStates.Reset();
    Super::Deinitialize();
}

bool UA302ServerPhaseSubsystem::StartRoomPhaseTimeline(const FString& RoomCode)
{
    const FString NormalizedRoomCode = A302RoomWorldOffset::NormalizeRoomCode(RoomCode);
    if (NormalizedRoomCode.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[A302ServerPhaseSubsystem] Cannot start room phase timeline with empty RoomCode."));
        return false;
    }

    const double CurrentServerTime = FPlatformTime::Seconds();

    FA302RoomPhaseState& RoomState = RoomPhaseStates.FindOrAdd(NormalizedRoomCode);
    const bool bRestartingExistingRoom = !RoomState.RoomCode.IsEmpty() && !RoomState.bFinished;

    RoomState.RoomCode = NormalizedRoomCode;
    RoomState.CurrentPhase = EGamePhase::Phase0;
    RoomState.MatchStartServerTime = static_cast<float>(CurrentServerTime);
    RoomState.PhaseChangedServerTime = static_cast<float>(CurrentServerTime);
    RoomState.bFinished = false;

    EnsurePhaseTimer();
    OnRoomPhaseChanged.Broadcast(NormalizedRoomCode, RoomState.CurrentPhase);

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[A302ServerPhaseSubsystem] %s room phase timeline. room=%s start=%.2f"),
        bRestartingExistingRoom ? TEXT("Restarted") : TEXT("Started"),
        *NormalizedRoomCode,
        CurrentServerTime
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
        return false;
    }

    if (!HasAnyActiveRoom())
    {
        if (UWorld* World = ResolveWorld())
        {
            World->GetTimerManager().ClearTimer(PhaseUpdateTimerHandle);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[A302ServerPhaseSubsystem] Stopped room timeline. room=%s"), *NormalizedRoomCode);
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

void UA302ServerPhaseSubsystem::UpdateRoomPhase(const FString& RoomCode, double CurrentServerTime)
{
    const FString NormalizedRoomCode = A302RoomWorldOffset::NormalizeRoomCode(RoomCode);
    FA302RoomPhaseState* RoomState = RoomPhaseStates.Find(NormalizedRoomCode);
    if (!RoomState || RoomState->bFinished)
    {
        return;
    }

    const double ElapsedSeconds = FMath::Max(0.0, CurrentServerTime - RoomState->MatchStartServerTime);
    const EGamePhase NewPhase = ResolvePhase(ElapsedSeconds);
    if (RoomState->CurrentPhase == NewPhase)
    {
        return;
    }

    RoomState->CurrentPhase = NewPhase;
    RoomState->PhaseChangedServerTime = static_cast<float>(CurrentServerTime);
    RoomState->bFinished = (NewPhase == EGamePhase::Ended);

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[A302ServerPhaseSubsystem] Room=%s phase changed to %s at %.2f (elapsed %.2f)"),
        *NormalizedRoomCode,
        ToString(NewPhase),
        CurrentServerTime,
        ElapsedSeconds
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
