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

    /** 가장 최근에 페이즈가 바뀐 방의 코드. 클라이언트에서 UI 필터링에 사용.
     *  ReplicatedUsing: GamePhase 값이 변하지 않아도(다른 방이 같은 페이즈로 전환)
     *  RoomCode 변경으로 OnRep이 발생해 Delegate를 다시 Broadcast할 수 있도록 함. */
    UPROPERTY(ReplicatedUsing = OnRep_GamePhaseRoomCode, BlueprintReadOnly)
    FString GamePhaseRoomCode;

    UPROPERTY(Replicated, BlueprintReadOnly)
    int32 AlivePlayerCount = 0;

    UPROPERTY(Replicated, BlueprintReadOnly)
    float PhaseChangedServerTime = 0.0f;

    /** 쾬 시작 시간(서버 절대시간). 클라이언트는 이 값으로 남은 시간을 계산합니다. */
    UPROPERTY(ReplicatedUsing = OnRep_MatchStartServerTime, BlueprintReadOnly)
    float MatchStartServerTime = 0.0f;

    /** 전체 게임 제한 시간(서버 설정 기준). */
    UPROPERTY(Replicated, BlueprintReadOnly)
    float MatchTimeLimitSeconds = 0.0f;

    void SetGamePhase(EGamePhase NewGamePhase, float ChangedServerTime, const FString& RoomCode = TEXT(""));
    void SetMatchTimer(float StartServerTime, float TimeLimitSeconds);

    UFUNCTION(BlueprintImplementableEvent, Category = "Phase")
    void Bp_OnEscapeUnlocked(const TArray<AActor*>& TargetActors);

    FOnGamePhaseChangedNative& OnGamePhaseChanged() { return GamePhaseChangedDelegate; }

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UFUNCTION()
    void OnRep_GamePhase();

    /** GamePhase 값이 동일할 때(다른 방이 같은 페이즈) OnRep_GamePhase는 발화하지 않음.
     *  GamePhaseRoomCode 변경을 감지해 같은 페이즈 다른 방 전환에도 Delegate를 Broadcast. */
    UFUNCTION()
    void OnRep_GamePhaseRoomCode();

    // MatchTimer RPC를 놓쳤거나 HUD 초기화 전에 도착한 경우를 위한 폴백.
    // MatchStartServerTime이 0→양수로 복제될 때 로컬 PC에 타이머를 재설정합니다.
    UFUNCTION()
    void OnRep_MatchStartServerTime();

    void HandlePhaseEnded();

    EGamePhase LastNotifiedGamePhase = EGamePhase::Phase0;
    /** OnRep_GamePhase 에서 이미 처리한 RoomCode를 기록해 OnRep_GamePhaseRoomCode 중복 발화를 방지. */
    FString LastNotifiedRoomCode;
    FOnGamePhaseChangedNative GamePhaseChangedDelegate;
};
