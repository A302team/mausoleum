// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/A302GameState.h"
#include "Net/UnrealNetwork.h"

AA302GameState::AA302GameState()
{
}

void AA302GameState::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AA302GameState, GamePhase);
    DOREPLIFETIME(AA302GameState, AlivePlayerCount);
}