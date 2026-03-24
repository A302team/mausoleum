// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "A302GameState.generated.h"

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    Phase0,
    Phase1,
    Phase2,
    Ended
};

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnGamePhaseChangedNative, EGamePhase, EGamePhase, float);

UCLASS()
class A302SHARED_API AA302GameState : public AGameState
{
    GENERATED_BODY()

public:
    AA302GameState();

    UPROPERTY(ReplicatedUsing = OnRep_GamePhase, BlueprintReadOnly)
    EGamePhase GamePhase = EGamePhase::Phase0;

    UPROPERTY(Replicated, BlueprintReadOnly)
    int32 AlivePlayerCount = 0;

    UPROPERTY(Replicated, BlueprintReadOnly)
    float PhaseChangedServerTime = 0.0f;

    void SetGamePhase(EGamePhase NewGamePhase, float ChangedServerTime);

    FOnGamePhaseChangedNative& OnGamePhaseChanged() { return GamePhaseChangedDelegate; }

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UFUNCTION()
    void OnRep_GamePhase();

    EGamePhase LastNotifiedGamePhase = EGamePhase::Phase0;
    FOnGamePhaseChangedNative GamePhaseChangedDelegate;
};
