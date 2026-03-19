#pragma once

#include "CoreMinimal.h"
#include "Room/RoomScopeRules.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

namespace A302GameplayGuards
{
	inline bool IsStandaloneLocalExecution(const UObject* WorldContextObject)
	{
		if (!WorldContextObject)
		{
			return false;
		}

		const UWorld* World = WorldContextObject->GetWorld();
		return World && World->GetNetMode() == NM_Standalone;
	}

	inline bool IsGameplayEnabledPlayer(const APlayerState* PlayerState)
	{
		if (IsStandaloneLocalExecution(PlayerState))
		{
			return true;
		}

		return A302RoomScope::IsPlayerGameplayEnabled(PlayerState);
	}

	inline bool IsGameplayEnabledCharacter(const ACharacter* Character)
	{
		return Character && IsGameplayEnabledPlayer(Character->GetPlayerState());
	}

	inline bool CanCharactersInteract(const ACharacter* InstigatorCharacter, const ACharacter* TargetCharacter)
	{
		if (!InstigatorCharacter || !TargetCharacter)
		{
			return false;
		}

		return A302RoomScope::ArePlayersInSameActiveLogicalRoom(
			InstigatorCharacter->GetPlayerState(),
			TargetCharacter->GetPlayerState()
		);
	}

	inline bool CanInstigatorAffectTargetActor(const ACharacter* InstigatorCharacter, const AActor* TargetActor)
	{
		if (!InstigatorCharacter || !IsGameplayEnabledCharacter(InstigatorCharacter))
		{
			return false;
		}

		if (!TargetActor)
		{
			return false;
		}

		if (const ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
		{
			return CanCharactersInteract(InstigatorCharacter, TargetCharacter);
		}

		return true;
	}

	inline bool DoesPlayerMatchEventRoom(const APlayerState* PlayerState, const FString& EventRoomCode)
	{
		if (!PlayerState)
		{
			return false;
		}

		return IsGameplayEnabledPlayer(PlayerState)
			&& A302RoomScope::MatchesRoomCode(EventRoomCode, A302RoomScope::ResolvePlayerRoomCode(PlayerState));
	}

	inline bool DoesControllerMatchEventRoom(const APlayerController* PlayerController, const FString& EventRoomCode)
	{
		return PlayerController && DoesPlayerMatchEventRoom(PlayerController->GetPlayerState<APlayerState>(), EventRoomCode);
	}
}

