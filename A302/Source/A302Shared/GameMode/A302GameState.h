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

    /** 쾬 시작 시간(서버 절대시간). 클라이언트는 이 값으로 남은 시간을 계산합니다. */
    UPROPERTY(Replicated, BlueprintReadOnly)
    float MatchStartServerTime = 0.0f;

    /** 전체 게임 제한 시간(서버 설정 기준). */
    UPROPERTY(Replicated, BlueprintReadOnly)
    float MatchTimeLimitSeconds = 0.0f;

    void SetGamePhase(EGamePhase NewGamePhase, float ChangedServerTime);
    void SetMatchTimer(float StartServerTime, float TimeLimitSeconds);

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UFUNCTION()
    void OnRep_GamePhase();
};
