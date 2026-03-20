// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/A302PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Room/RoomWorldOffset.h"
#include "Character/MyCharacter.h"

AA302PlayerState::AA302PlayerState()
{
}

void AA302PlayerState::SetRoomCode(const FString& InRoomCode)
{
    if (!HasAuthority())
    {
        return;
    }

    RoomCode = A302RoomWorldOffset::NormalizeRoomCode(InRoomCode);
}

void AA302PlayerState::SetGameplayEnabled(bool bInGameplayEnabled)
{
    if (!HasAuthority())
    {
        return;
    }

    bGameplayEnabled = bInGameplayEnabled;
}

void AA302PlayerState::OnRep_IsAlive()
{
    if (!bIsAlive)
    {
        if (AMyCharacter* OwningCharacter = Cast<AMyCharacter>(GetPawn()))
        {
            OwningCharacter->ForceDeathByPersonalEvent();
        }
    }
}

void AA302PlayerState::OnRep_RoomCode()
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
    DOREPLIFETIME(AA302PlayerState, RoomCode);
    DOREPLIFETIME(AA302PlayerState, bGameplayEnabled);
}
