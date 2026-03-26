#include "Phase/A302ServerPhaseSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Object/StatueInteractable.h"
#include "Object/EscapeRouteBlocker.h"
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
    RoomState.MatchStartServerTime = 0.0f;  // 타이머는 캐릭터 스폰 후 NotifyRoomMatchTimerStart()에서 시작
    RoomState.PhaseChangedServerTime = static_cast<float>(CurrentServerTime);
    RoomState.Phase0ItemCount = 0;
    RoomState.Phase1ClearObjectCount = 0;
    RoomState.Phase2GroupEventCount = 0;
    RoomState.bTimedOutWithoutEscape = false;
    RoomState.bMatchTimerStarted = false;
    RoomState.bFinished = false;

    EnsurePhaseTimer();
    BroadcastPhaseClearProgressToRoom(NormalizedRoomCode, RoomState, true);
    BroadcastMatchTimerStateToRoom(NormalizedRoomCode, 0.0f, MatchTimeLimitSeconds, false); // 타이머 UI 숨김
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
            // 모든 석상이 완료된 순간: 나머지 석상 강제완료 + EscapeRouteBlocker 해제
            if (RoomState->Phase2GroupEventCount >= FMath::Max(1, Phase2RequiredGroupEventCount))
            {
                TriggerAllStatuesCompleteInRoom(NormalizedRoomCode);
            }
        }
        break;
    default:
        break;
    }

    if (bCounted)
    {
        BroadcastPhaseClearProgressToRoom(NormalizedRoomCode, *RoomState, true);
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

bool UA302ServerPhaseSubsystem::NotifyRoomMatchTimerStart(const FString& RoomCode)
{
    const FString NormalizedRoomCode = A302RoomWorldOffset::NormalizeRoomCode(RoomCode);
    FA302RoomPhaseState* RoomState = RoomPhaseStates.Find(NormalizedRoomCode);
    if (!RoomState || RoomState->bFinished)
    {
        UE_LOG(LogA302Phase, Warning, TEXT("NotifyRoomMatchTimerStart: room not found or finished. room=%s"), *NormalizedRoomCode);
        return false;
    }

    if (RoomState->bMatchTimerStarted)
    {
        UE_LOG(LogA302Phase, Verbose, TEXT("NotifyRoomMatchTimerStart: timer already started. room=%s"), *NormalizedRoomCode);
        return false;
    }

    const double CurrentServerTime = FPlatformTime::Seconds();
    RoomState->MatchStartServerTime = static_cast<float>(CurrentServerTime);
    RoomState->bMatchTimerStarted = true;

    // 클라이언트 HUD는 GetServerWorldTimeSeconds() 기반으로 남은 시간을 계산하지만,
    // 서버 내부 페이즈 평가는 FPlatformTime::Seconds() 기반입니다.
    // 두 시계는 원점(기준 시각)이 다르기 때문에, 클라이언트에 전달할 start time은
    // GetServerWorldTimeSeconds() 기준으로 따로 구해야 합니다.
    float HUDMatchStartTime = static_cast<float>(CurrentServerTime); // 기본값 (fallback)
    if (UWorld* World = ResolveWorld())
    {
        if (const AGameStateBase* GS = World->GetGameState())
        {
            HUDMatchStartTime = GS->GetServerWorldTimeSeconds();
        }
    }

    // 클라이언트 타이머 UI 표시
    BroadcastMatchTimerStateToRoom(NormalizedRoomCode, HUDMatchStartTime, MatchTimeLimitSeconds, MatchTimeLimitSeconds > 0.0f);
    BroadcastPhaseClearProgressToRoom(NormalizedRoomCode, *RoomState, true);

    // GameState 복제 → 클라이언트가 서버 시간 참조 가능
    if (UWorld* World = ResolveWorld())
    {
        if (AA302GameState* GS = Cast<AA302GameState>(World->GetGameState()))
        {
            GS->SetMatchTimer(HUDMatchStartTime, MatchTimeLimitSeconds);
        }
    }

    UE_LOG(
        LogA302Phase,
        Log,
        TEXT("Match timer STARTED. room=%s startTime=%.2f limitSeconds=%.0f"),
        *NormalizedRoomCode,
        RoomState->MatchStartServerTime,
        MatchTimeLimitSeconds
    );

    return true;
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
    const EGamePhase NewPhase = ResolvePhase(*RoomState, CurrentServerTime);
    if (PreviousPhase == NewPhase)
    {
        return;
    }

    FString TransitionReason = TEXT("rule satisfied");
    const bool bTimedOutWithoutEscape =
        MatchTimeLimitSeconds > 0.0f &&
        (CurrentServerTime - RoomState->MatchStartServerTime) >= MatchTimeLimitSeconds &&
        CountEscapedPlayersInRoom(RoomState->RoomCode) <= 0;

    if (PreviousPhase == EGamePhase::Phase0 && NewPhase == EGamePhase::Phase1)
    {
        TransitionReason = FString::Printf(TEXT("Phase0 item count reached (%d/%d)"), RoomState->Phase0ItemCount, FMath::Max(0, Phase0RequiredItemCount));
    }
    else if (PreviousPhase == EGamePhase::Phase1 && NewPhase == EGamePhase::Phase2)
    {
        TransitionReason = FString::Printf(TEXT("Phase1 clear object count reached (%d/%d)"), RoomState->Phase1ClearObjectCount, FMath::Max(0, Phase1RequiredClearObjectCount));
    }
    else if (PreviousPhase == EGamePhase::Phase2 && NewPhase == EGamePhase::Ended)
    {
        const bool bIsTimeout = MatchTimeLimitSeconds > 0.0f && (CurrentServerTime - RoomState->MatchStartServerTime) >= MatchTimeLimitSeconds;
        if (bIsTimeout)
        {
            TransitionReason = FString::Printf(
                TEXT("Match time limit reached (elapsed=%.0fs, limit=%.0fs)"),
                CurrentServerTime - RoomState->MatchStartServerTime,
                MatchTimeLimitSeconds
            );
        }
        else
        {
            TransitionReason = FString::Printf(
                TEXT("Escape count reached (%d/%d)"),
                CountEscapedPlayersInRoom(RoomState->RoomCode),
                FMath::Max(1, EscapeRequiredCount)
            );
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

    BroadcastPhaseClearProgressToRoom(NormalizedRoomCode, *RoomState, !RoomState->bFinished);

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

EGamePhase UA302ServerPhaseSubsystem::ResolvePhase(const FA302RoomPhaseState& RoomState, double CurrentServerTime) const
{
    // 타이머가 시작되지 않았다면 어떤 페이즈 전환(시간 만료 등)도 처리하지 않고 홀딩
    if (!RoomState.bMatchTimerStarted)
    {
        return RoomState.CurrentPhase;
    }

    if (MatchTimeLimitSeconds > 0.0f)
    {
        const double ElapsedSeconds = CurrentServerTime - RoomState.MatchStartServerTime;
        if (ElapsedSeconds >= MatchTimeLimitSeconds)
        {
            return EGamePhase::Ended;
        }
    }

    const float Elapsed = static_cast<float>(CurrentServerTime - RoomState.PhaseChangedServerTime);

    switch (RoomState.CurrentPhase)
    {
    case EGamePhase::Phase0:
    {
        // 미션을 완료해서 넘어가야함
        const int32 RequiredItemCount = FMath::Max(0, Phase0RequiredItemCount);
        const bool bCountReached = RequiredItemCount <= 0 || RoomState.Phase0ItemCount >= RequiredItemCount;
        if (bCountReached)
        {
            return EGamePhase::Phase1;
        }
        return EGamePhase::Phase0;
    }
    case EGamePhase::Phase1:
    {
        // 미션을 완료해서 넘어가야함
        const int32 RequiredClearCount = FMath::Max(0, Phase1RequiredClearObjectCount);
        const bool bCountReached = RequiredClearCount <= 0 || RoomState.Phase1ClearObjectCount >= RequiredClearCount;
        if (bCountReached)
        {
            return EGamePhase::Phase2;
        }
        return EGamePhase::Phase1;
    }
    case EGamePhase::Phase2:
    {
        // 석상 미션 완료는 포탈 접근을 열어주는 역할만 함 (별도 이벤트로 처리).
        // Phase2 → Ended 전환은 포탈로 탈출한 플레이어 수 기준.
        const int32 RequiredEscapes = FMath::Max(1, EscapeRequiredCount);
        if (CountEscapedPlayersInRoom(RoomState.RoomCode) >= RequiredEscapes)
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

void UA302ServerPhaseSubsystem::BroadcastPhaseClearProgressToRoom(const FString& RoomCode, const FA302RoomPhaseState& RoomState, bool bVisible) const
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

    int32 CurrentCount = 0;
    int32 RequiredCount = 0;
    switch (RoomState.CurrentPhase)
    {
    case EGamePhase::Phase0:
        CurrentCount = RoomState.Phase0ItemCount;
        RequiredCount = Phase0RequiredItemCount;
        break;
    case EGamePhase::Phase1:
        CurrentCount = RoomState.Phase1ClearObjectCount;
        RequiredCount = Phase1RequiredClearObjectCount;
        break;
    case EGamePhase::Phase2:
        CurrentCount = RoomState.Phase2GroupEventCount;
        RequiredCount = Phase2RequiredGroupEventCount;
        break;
    default:
        break;
    }

    TArray<APlayerController*> Players;
    Registry->GatherPlayersInRoom(World, RoomCode, Players);

    const bool bShouldShow = bVisible && RoomState.CurrentPhase != EGamePhase::Ended;
    for (APlayerController* PlayerController : Players)
    {
        if (AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(PlayerController))
        {
            MyPlayerController->UpdatePhaseClearProgress(
                static_cast<uint8>(RoomState.CurrentPhase),
                FMath::Max(0, CurrentCount),
                FMath::Max(0, RequiredCount),
                bShouldShow
            );
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

void UA302ServerPhaseSubsystem::TriggerAllStatuesCompleteInRoom(const FString& RoomCode)
{
    UWorld* World = ResolveWorld();
    if (!World)
    {
        return;
    }

    // 룸 중심 X 좌표로 같은 룸의 액터만 필터링
    const FVector RoomCenter = A302RoomWorldOffset::ResolveRoomOffset(RoomCode);
    const float HalfStep = static_cast<float>(A302RoomWorldOffset::DefaultOffsetStepX * 0.5);

    // 룸 내 미완료 석상 모두 강제 완료 (이펙트 재생 포함)
    TArray<AActor*> StatueActors;
    UGameplayStatics::GetAllActorsOfClass(World, AStatueInteractable::StaticClass(), StatueActors);
    for (AActor* Actor : StatueActors)
    {
        if (!Actor || FMath::Abs(Actor->GetActorLocation().X - RoomCenter.X) > HalfStep)
        {
            continue;
        }
        if (AStatueInteractable* Statue = Cast<AStatueInteractable>(Actor))
        {
            Statue->ForceComplete();
        }
    }

    // 룸 내 EscapeRouteBlocker 모두 해제 (탈출로 오픈)
    TArray<AActor*> BlockerActors;
    UGameplayStatics::GetAllActorsOfClass(World, AEscapeRouteBlocker::StaticClass(), BlockerActors);
    for (AActor* Actor : BlockerActors)
    {
        if (!Actor || FMath::Abs(Actor->GetActorLocation().X - RoomCenter.X) > HalfStep)
        {
            continue;
        }
        if (AEscapeRouteBlocker* Blocker = Cast<AEscapeRouteBlocker>(Actor))
        {
            Blocker->OpenEscapeRoute();
        }
    }

    UE_LOG(LogA302Phase, Log,
        TEXT("TriggerAllStatuesComplete: forced statues=%d blockers=%d room=%s"),
        StatueActors.Num(), BlockerActors.Num(), *RoomCode);
}
