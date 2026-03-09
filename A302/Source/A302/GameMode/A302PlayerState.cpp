// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/A302PlayerState.h"
#include "Net/UnrealNetwork.h"

AA302PlayerState::AA302PlayerState()
{
}

void AA302PlayerState::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AA302PlayerState, PlayerRole);
    DOREPLIFETIME(AA302PlayerState, bIsAlive);
    DOREPLIFETIME(AA302PlayerState, bIsEscaped);
    DOREPLIFETIME(AA302PlayerState, bHasKillItem);
}
