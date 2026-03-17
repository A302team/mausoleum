#include "Room/RoomMembershipRegistry.h"

#include "GameMode/A302PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Room/RoomScopeRules.h"

namespace
{
	FString ExtractOptionValueFallback(const FString& Options, const FString& OptionKey)
	{
		if (Options.IsEmpty() || OptionKey.IsEmpty())
		{
			return FString();
		}

		TArray<FString> Tokens;
		const TCHAR* Delimiters[] = { TEXT("?"), TEXT("&") };
		Options.ParseIntoArray(Tokens, Delimiters, UE_ARRAY_COUNT(Delimiters), true);
		for (const FString& Token : Tokens)
		{
			FString Key;
			FString Value;
			if (!Token.Split(TEXT("="), &Key, &Value))
			{
				continue;
			}

			if (Key.Equals(OptionKey, ESearchCase::IgnoreCase))
			{
				return Value;
			}
		}

		return FString();
	}
}

void URoomMembershipRegistry::RegisterPendingRoomCode(APlayerController* PlayerController, const FString& Options)
{
	if (!PlayerController)
	{
		return;
	}

	FString RequestedRoomCode = UGameplayStatics::ParseOption(Options, TEXT("roomCode"));
	if (RequestedRoomCode.IsEmpty())
	{
		RequestedRoomCode = UGameplayStatics::ParseOption(Options, TEXT("RoomCode"));
	}
	if (RequestedRoomCode.IsEmpty())
	{
		RequestedRoomCode = ExtractOptionValueFallback(Options, TEXT("roomCode"));
	}
	RequestedRoomCode = A302RoomScope::NormalizeRoomCode(RequestedRoomCode);

	PendingRoomCodeByController.Add(PlayerController, RequestedRoomCode);
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[RoomMembershipRegistry] Register pending room. player=%s room=%s"),
		*GetNameSafe(PlayerController),
		*RequestedRoomCode
	);
}

void URoomMembershipRegistry::ClearPendingRoomCode(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	PendingRoomCodeByController.Remove(PlayerController);
}

FString URoomMembershipRegistry::ResolveInitialRoomCode(APlayerController* PlayerController) const
{
	if (!PlayerController)
	{
		return TEXT("");
	}

	if (const FString* PendingRoomCode = PendingRoomCodeByController.Find(PlayerController))
	{
		if (!PendingRoomCode->IsEmpty())
		{
			return A302RoomScope::NormalizeRoomCode(*PendingRoomCode);
		}
	}

	return A302RoomScope::NormalizeRoomCode(TEXT("default"));
}

void URoomMembershipRegistry::AssignPlayerToRoom(APlayerController* PlayerController)
{
	if (!PlayerController || !PlayerController->HasAuthority())
	{
		return;
	}

	AA302PlayerState* A302PlayerState = PlayerController->GetPlayerState<AA302PlayerState>();
	if (!A302PlayerState)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[RoomMembershipRegistry] Assign room skipped: AA302PlayerState missing. player=%s playerStateClass=%s"),
			*GetNameSafe(PlayerController),
			PlayerController->PlayerState ? *PlayerController->PlayerState->GetClass()->GetName() : TEXT("None")
		);
		return;
	}

	const FString CurrentRoomCode = A302RoomScope::NormalizeRoomCode(A302PlayerState->GetRoomCode());
	const bool bHasPendingRoomCode = PendingRoomCodeByController.Contains(PlayerController);
	if (!bHasPendingRoomCode && !CurrentRoomCode.IsEmpty())
	{
		// PostLogin/HandleStartingNewPlayer can both run for the same controller.
		// Do not overwrite an already assigned logical room with fallback "default".
		return;
	}

	const FString AssignedRoomCode = A302RoomScope::NormalizeRoomCode(ResolveInitialRoomCode(PlayerController));
	if (AssignedRoomCode.IsEmpty() || A302RoomScope::MatchesRoomCodeStrict(CurrentRoomCode, AssignedRoomCode))
	{
		return;
	}

	A302PlayerState->SetRoomCode(AssignedRoomCode);
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[RoomMembershipRegistry] Assign room. player=%s room=%s"),
		*GetNameSafe(PlayerController),
		*AssignedRoomCode
	);

	PendingRoomCodeByController.Remove(PlayerController);
}

FString URoomMembershipRegistry::GetPlayerRoomCode(const APlayerController* PlayerController) const
{
	if (!PlayerController)
	{
		return FString();
	}

	if (const AA302PlayerState* A302PlayerState = PlayerController->GetPlayerState<AA302PlayerState>())
	{
		return A302RoomScope::NormalizeRoomCode(A302PlayerState->GetRoomCode());
	}

	return FString();
}

bool URoomMembershipRegistry::IsPlayerInRoom(const APlayerController* PlayerController, const FString& RoomCode) const
{
	const FString NormalizedRoomCode = A302RoomScope::NormalizeRoomCode(RoomCode);
	if (!PlayerController || NormalizedRoomCode.IsEmpty())
	{
		return false;
	}

	return A302RoomScope::MatchesRoomCodeStrict(GetPlayerRoomCode(PlayerController), NormalizedRoomCode);
}

void URoomMembershipRegistry::GatherPlayersInRoom(UWorld* World, const FString& RoomCode, TArray<APlayerController*>& OutPlayers) const
{
	OutPlayers.Reset();
	const FString NormalizedRoomCode = A302RoomScope::NormalizeRoomCode(RoomCode);
	if (!World || NormalizedRoomCode.IsEmpty())
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (IsPlayerInRoom(PlayerController, NormalizedRoomCode))
		{
			OutPlayers.Add(PlayerController);
		}
	}
}

int32 URoomMembershipRegistry::CountPlayersInRoom(UWorld* World, const FString& RoomCode) const
{
	const FString NormalizedRoomCode = A302RoomScope::NormalizeRoomCode(RoomCode);
	if (!World || NormalizedRoomCode.IsEmpty())
	{
		return 0;
	}

	int32 Count = 0;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (IsPlayerInRoom(It->Get(), NormalizedRoomCode))
		{
			++Count;
		}
	}

	return Count;
}
