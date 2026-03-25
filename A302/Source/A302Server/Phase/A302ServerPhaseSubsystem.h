#pragma once

#include "CoreMinimal.h"
#include "GameMode/A302GameState.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "A302ServerPhaseSubsystem.generated.h"

enum class ERewardCategory : uint8;
class URewardDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRoomPhaseChanged, const FString&, RoomCode, EGamePhase, NewPhase);

USTRUCT(BlueprintType)
struct FA302RoomPhaseState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString RoomCode;

    UPROPERTY(BlueprintReadOnly)
    EGamePhase CurrentPhase = EGamePhase::Phase0;

    UPROPERTY(BlueprintReadOnly)
    float MatchStartServerTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float PhaseChangedServerTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    int32 Phase0ItemCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 Phase1ClearObjectCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 Phase2GroupEventCount = 0;

    UPROPERTY(BlueprintReadOnly)
    bool bTimedOutWithoutEscape = false;

    UPROPERTY(BlueprintReadOnly)
    bool bMatchTimerStarted = false;

    UPROPERTY(BlueprintReadOnly)
    bool bFinished = false;
};

UCLASS(Config = Game)
class A302SERVER_API UA302ServerPhaseSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UPROPERTY(Config, EditAnywhere, Category = "Phase")
    float Phase0Duration = 10.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Phase")
    float Phase1Duration = 10.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Phase")
    float Phase2Duration = 30.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Phase")
    int32 Phase0RequiredItemCount = 1;

    UPROPERTY(Config, EditAnywhere, Category = "Phase")
    int32 Phase1RequiredClearObjectCount = 1;

    UPROPERTY(Config, EditAnywhere, Category = "Phase")
    int32 Phase2RequiredGroupEventCount = 1;

    UPROPERTY(Config, EditAnywhere, Category = "Phase")
    float MatchTimeLimitSeconds = 666.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Phase")
    float PhasePollInterval = 0.25f;

    UPROPERTY(Config, EditAnywhere, Category = "Phase|Log")
    bool bLogPhasePolling = false;

    UPROPERTY(BlueprintAssignable, Category = "Phase")
    FOnRoomPhaseChanged OnRoomPhaseChanged;

    UFUNCTION(BlueprintCallable, Category = "Phase")
    bool StartRoomPhaseTimeline(const FString& RoomCode);

    UFUNCTION(BlueprintCallable, Category = "Phase")
    bool StopRoomPhaseTimeline(const FString& RoomCode);

    UFUNCTION(BlueprintPure, Category = "Phase")
    bool TryGetRoomPhaseState(const FString& RoomCode, FA302RoomPhaseState& OutRoomState) const;

    UFUNCTION(BlueprintPure, Category = "Phase")
    bool IsRoomPhaseActive(const FString& RoomCode) const;

    UFUNCTION(BlueprintCallable, Category = "Phase")
    void NotifyRoomRewardResolved(const FString& RoomCode, const URewardDefinition* RewardDefinition, ERewardCategory RewardCategory);

    /**
     * @brief 캐릭터가 스폰된 시점에 호출하여 전체 게임 타이머를 시작합니다.
     * StartRoomPhaseTimeline 코드 이후 SpawnPlayersInRoom에서 호출해야 합니다.
     */
    UFUNCTION(BlueprintCallable, Category = "Phase")
    bool NotifyRoomMatchTimerStart(const FString& RoomCode);

private:
    void HandleMapLoaded(UWorld* LoadedWorld);
    void EvaluateRoomPhases();
    void EnsurePhaseTimer();
    void BroadcastMatchTimerStateToRoom(const FString& RoomCode, float MatchStartServerTime, float DurationSeconds, bool bVisible) const;
    void BroadcastPhaseClearProgressToRoom(const FString& RoomCode, const FA302RoomPhaseState& RoomState, bool bVisible) const;
    void UpdateRoomPhase(const FString& RoomCode, double CurrentServerTime);
    bool HasAnyActiveRoom() const;
    UWorld* ResolveWorld() const;
    EGamePhase ResolvePhase(const FA302RoomPhaseState& RoomState, double CurrentServerTime) const;
    int32 CountAlivePlayersInRoom(const FString& RoomCode) const;
    int32 CountEscapedPlayersInRoom(const FString& RoomCode) const;
    FString BuildRoomProgressSummary(const FA302RoomPhaseState& RoomState) const;
    static const TCHAR* ToString(EGamePhase Phase);
    static const TCHAR* ToString(ERewardCategory RewardCategory);

    FTimerHandle PhaseUpdateTimerHandle;

    UPROPERTY()
    TMap<FString, FA302RoomPhaseState> RoomPhaseStates;
};
