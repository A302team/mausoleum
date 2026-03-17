#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "GameFramework/PlayerState.h"
#include "GameMode/A302PlayerState.h"
#include "GamePlay/Events/BaseEvent.h"
#include "Network/LobbyConstants.h"
#include "Room/RoomWorldOffset.h"

namespace A302RoomScope
{
	inline FString NormalizeRoomCode(const FString& RoomCode)
	{
		return A302RoomWorldOffset::NormalizeRoomCode(RoomCode);
	}

	inline bool MatchesRoomCodeStrict(const FString& LeftRoomCode, const FString& RightRoomCode)
	{
		const FString NormalizedLeft = NormalizeRoomCode(LeftRoomCode);
		const FString NormalizedRight = NormalizeRoomCode(RightRoomCode);
		if (NormalizedLeft.IsEmpty() || NormalizedRight.IsEmpty())
		{
			return false;
		}

		return NormalizedLeft == NormalizedRight;
	}

	inline FString ExtractRoomCode(const TSharedPtr<FJsonObject>& Data)
	{
		if (!Data.IsValid())
		{
			return FString();
		}

		FString RoomCode;
		Data->TryGetStringField(LobbyProtocol::KeyRoomCode, RoomCode);
		return NormalizeRoomCode(RoomCode);
	}

	inline bool MatchesRoomCode(const FString& ExpectedRoomCode, const FString& ActualRoomCode)
	{
		const FString NormalizedExpected = NormalizeRoomCode(ExpectedRoomCode);
		const FString NormalizedActual = NormalizeRoomCode(ActualRoomCode);
		if (NormalizedExpected.IsEmpty() || NormalizedActual.IsEmpty())
		{
			return true;
		}

		return NormalizedExpected == NormalizedActual;
	}

	inline bool IsMessageForRoom(const FString& CurrentRoomCode, const TSharedPtr<FJsonObject>& Data, FString* OutRoomCode = nullptr)
	{
		const FString MessageRoomCode = ExtractRoomCode(Data);
		if (OutRoomCode)
		{
			*OutRoomCode = MessageRoomCode;
		}

		return MatchesRoomCode(CurrentRoomCode, MessageRoomCode);
	}

	inline FString ResolvePlayerRoomCode(const APlayerState* PlayerState)
	{
		if (const AA302PlayerState* A302PlayerState = Cast<AA302PlayerState>(PlayerState))
		{
			return NormalizeRoomCode(A302PlayerState->GetRoomCode());
		}

		return FString();
	}

	inline bool ArePlayersInSameLogicalRoom(const APlayerState* PlayerStateA, const APlayerState* PlayerStateB)
	{
		return MatchesRoomCodeStrict(ResolvePlayerRoomCode(PlayerStateA), ResolvePlayerRoomCode(PlayerStateB));
	}

	inline bool IsPlayerGameplayEnabled(const APlayerState* PlayerState)
	{
		if (const AA302PlayerState* A302PlayerState = Cast<AA302PlayerState>(PlayerState))
		{
			return A302PlayerState->bGameplayEnabled;
		}

		return false;
	}

	inline bool ArePlayersInSameActiveLogicalRoom(const APlayerState* PlayerStateA, const APlayerState* PlayerStateB)
	{
		if (!ArePlayersInSameLogicalRoom(PlayerStateA, PlayerStateB))
		{
			return false;
		}

		return IsPlayerGameplayEnabled(PlayerStateA) && IsPlayerGameplayEnabled(PlayerStateB);
	}

	inline bool DoesEventBelongToPlayerRoom(const APlayerState* PlayerState, const UBaseEvent* Event)
	{
		if (!Event)
		{
			return true;
		}

		return MatchesRoomCode(ResolvePlayerRoomCode(PlayerState), Event->GetEventRoomCode());
	}
}
