// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/A302GameState.h"
#include "Net/UnrealNetwork.h"

AA302GameState::AA302GameState()
{
}

void AA302GameState::SetGamePhase(EGamePhase NewGamePhase, float ChangedServerTime)
{
    if (GamePhase == NewGamePhase && FMath::IsNearlyEqual(PhaseChangedServerTime, ChangedServerTime))
    {
        return;
    }

    GamePhase = NewGamePhase;
    PhaseChangedServerTime = ChangedServerTime;
}

void AA302GameState::SetMatchTimer(float StartServerTime, float TimeLimitSeconds)
{
    MatchStartServerTime = StartServerTime;
    MatchTimeLimitSeconds = TimeLimitSeconds;
}

void AA302GameState::OnRep_GamePhase()
{
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
