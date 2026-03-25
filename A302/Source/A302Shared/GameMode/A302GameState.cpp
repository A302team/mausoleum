// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/A302GameState.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Object/StatueInteractable.h"

AA302GameState::AA302GameState()
{
    LastNotifiedGamePhase = GamePhase;
}

void AA302GameState::SetGamePhase(EGamePhase NewGamePhase, float ChangedServerTime)
{
    if (GamePhase == NewGamePhase && FMath::IsNearlyEqual(PhaseChangedServerTime, ChangedServerTime))
    {
        return;
    }

    GamePhase = NewGamePhase;
    PhaseChangedServerTime = ChangedServerTime;
    LastNotifiedGamePhase = GamePhase;

    if (GamePhase == EGamePhase::Ended)
    {
        HandlePhaseEnded();
    }
}

void AA302GameState::SetMatchTimer(float StartServerTime, float TimeLimitSeconds)
{
    MatchStartServerTime = StartServerTime;
    MatchTimeLimitSeconds = TimeLimitSeconds;
}

void AA302GameState::OnRep_GamePhase()
{
    const EGamePhase PreviousPhase = LastNotifiedGamePhase;
    LastNotifiedGamePhase = GamePhase;

    if (PreviousPhase != GamePhase)
    {
        GamePhaseChangedDelegate.Broadcast(PreviousPhase, GamePhase, PhaseChangedServerTime);

        if (GamePhase == EGamePhase::Ended)
        {
            HandlePhaseEnded();
        }
    }
}

void AA302GameState::HandlePhaseEnded()
{
    // 모든 미완료 석상들을 찾아서 100%로 강제 완료 처리 (서버 전용)
    if (HasAuthority())
    {
        TArray<AActor*> FoundStatues;
        UGameplayStatics::GetAllActorsOfClass(this, AStatueInteractable::StaticClass(), FoundStatues);
        for (AActor* Actor : FoundStatues)
        {
            if (AStatueInteractable* Statue = Cast<AStatueInteractable>(Actor))
            {
                Statue->ForceComplete();
            }
        }
    }

    // Stage3ClearOff 태그를 가진 액터들을 찾아서 탈출구를 개방
    TArray<AActor*> TargetActors;
    UGameplayStatics::GetAllActorsWithTag(this, TEXT("Stage3ClearOff"), TargetActors);

    // 물리적 충돌은 더 이상 막지 않도록 서버에서 끕니다
    if (HasAuthority())
    {
        for (AActor* Actor : TargetActors)
        {
            if (Actor)
            {
                Actor->SetActorEnableCollision(false);
            }
        }
    }

    // 클라이언트와 서버 모두 BP 이벤트 호출 (BP에서 Local Fog 볼륨 등의 시각적 페이드아웃 처리)
    Bp_OnEscapeUnlocked(TargetActors);
}

void AA302GameState::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AA302GameState, GamePhase);
    DOREPLIFETIME(AA302GameState, AlivePlayerCount);
    DOREPLIFETIME(AA302GameState, PhaseChangedServerTime);
    DOREPLIFETIME(AA302GameState, MatchStartServerTime);
    DOREPLIFETIME(AA302GameState, MatchTimeLimitSeconds);
}
