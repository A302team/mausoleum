#pragma once

#include "CoreMinimal.h"
#include "GameMode/A302GameState.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "A302ServerPhaseSubsystem.generated.h"

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
    float Phase0Duration = 30.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Phase")
    float Phase1Duration = 30.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Phase")
    float Phase2Duration = 30.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Phase")
    float PhasePollInterval = 0.25f;

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

private:
    void HandleMapLoaded(UWorld* LoadedWorld);
    void EvaluateRoomPhases();
    void EnsurePhaseTimer();
    void UpdateRoomPhase(const FString& RoomCode, double CurrentServerTime);
    bool HasAnyActiveRoom() const;
    UWorld* ResolveWorld() const;
    EGamePhase ResolvePhase(double ElapsedSeconds) const;
    static const TCHAR* ToString(EGamePhase Phase);

    FTimerHandle PhaseUpdateTimerHandle;

    UPROPERTY()
    TMap<FString, FA302RoomPhaseState> RoomPhaseStates;
};
