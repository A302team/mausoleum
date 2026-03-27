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

	bool IsLocalPieSession(const APlayerController* PlayerController)
	{
#if WITH_EDITOR
		if (!PlayerController)
		{
			return false;
		}

		const UWorld* World = PlayerController->GetWorld();
		return World && World->WorldType == EWorldType::PIE && World->GetNetMode() != NM_DedicatedServer;
#else
		return false;
#endif
	}

	FString BuildLocalPieRoomCode(const APlayerController* PlayerController)
	{
		const UWorld* World = PlayerController ? PlayerController->GetWorld() : nullptr;
		if (!World)
		{
			return FString();
		}

		FString SafeMapName = World->GetMapName();
		int32 PieInstance = 0;
		if (SafeMapName.StartsWith(TEXT("UEDPIE_")))
		{
			const int32 PrefixLen = 7; // "UEDPIE_"
			const int32 UnderscoreIndex = SafeMapName.Find(TEXT("_"), ESearchCase::IgnoreCase, ESearchDir::FromStart, PrefixLen);
			if (UnderscoreIndex != INDEX_NONE)
			{
				const FString PieInstanceToken = SafeMapName.Mid(PrefixLen, UnderscoreIndex - PrefixLen);
				PieInstance = FMath::Max(0, FCString::Atoi(*PieInstanceToken));
				SafeMapName = SafeMapName.Mid(UnderscoreIndex + 1);
			}
		}
		for (TCHAR& Ch : SafeMapName)
		{
			if (!FChar::IsAlnum(Ch))
			{
				Ch = TEXT('_');
			}
		}

		const FString GeneratedCode = FString::Printf(TEXT("PIE_LOCAL_%d_%s"), PieInstance, *SafeMapName);
		return A302RoomScope::NormalizeRoomCode(GeneratedCode);
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

void URoomMembershipRegistry::UnregisterPlayer(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	ClearPendingRoomCode(PlayerController);
	UntrackPlayerRoom(PlayerController);
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

	if (IsLocalPieSession(PlayerController))
	{
		const FString GeneratedRoomCode = BuildLocalPieRoomCode(PlayerController);
		if (!GeneratedRoomCode.IsEmpty())
		{
			return GeneratedRoomCode;
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
		TrackPlayerRoom(PlayerController, CurrentRoomCode);
		return;
	}

	const FString AssignedRoomCode = A302RoomScope::NormalizeRoomCode(ResolveInitialRoomCode(PlayerController));
	if (AssignedRoomCode.IsEmpty())
	{
		return;
	}

	if (A302RoomScope::MatchesRoomCodeStrict(CurrentRoomCode, AssignedRoomCode))
	{
		TrackPlayerRoom(PlayerController, AssignedRoomCode);
		PendingRoomCodeByController.Remove(PlayerController);
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

	if (IsLocalPieSession(PlayerController) && AssignedRoomCode.StartsWith(TEXT("PIE_LOCAL_")))
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[RoomMembershipRegistry] Local PIE fallback room assigned. player=%s room=%s"),
			*GetNameSafe(PlayerController),
			*AssignedRoomCode
		);
	}

	TrackPlayerRoom(PlayerController, AssignedRoomCode);
	PendingRoomCodeByController.Remove(PlayerController);
}

FString URoomMembershipRegistry::GetPlayerRoomCode(const APlayerController* PlayerController) const
{
	if (!PlayerController)
	{
		return FString();
	}

	const TWeakObjectPtr<APlayerController> WeakPlayerController(const_cast<APlayerController*>(PlayerController));
	if (const FString* CachedRoomCode = AssignedRoomCodeByController.Find(WeakPlayerController))
	{
		return A302RoomScope::NormalizeRoomCode(*CachedRoomCode);
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

	if (const TSet<TWeakObjectPtr<APlayerController>>* RoomPlayers = PlayersByRoomCode.Find(NormalizedRoomCode))
	{
		for (const TWeakObjectPtr<APlayerController>& WeakPlayerController : *RoomPlayers)
		{
			APlayerController* PlayerController = WeakPlayerController.Get();
			if (!PlayerController || PlayerController->GetWorld() != World)
			{
				continue;
			}

			if (IsPlayerInRoom(PlayerController, NormalizedRoomCode))
			{
				OutPlayers.Add(PlayerController);
			}
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

	if (const TSet<TWeakObjectPtr<APlayerController>>* RoomPlayers = PlayersByRoomCode.Find(NormalizedRoomCode))
	{
		int32 Count = 0;
		for (const TWeakObjectPtr<APlayerController>& WeakPlayerController : *RoomPlayers)
		{
			APlayerController* PlayerController = WeakPlayerController.Get();
			if (!PlayerController || PlayerController->GetWorld() != World)
			{
				continue;
			}

			if (IsPlayerInRoom(PlayerController, NormalizedRoomCode))
			{
				++Count;
			}
		}
		return Count;
	}

	return 0;
}

void URoomMembershipRegistry::TrackPlayerRoom(APlayerController* PlayerController, const FString& RoomCode)
{
	if (!PlayerController)
	{
		return;
	}

	const FString NormalizedRoomCode = A302RoomScope::NormalizeRoomCode(RoomCode);
	if (NormalizedRoomCode.IsEmpty())
	{
		UntrackPlayerRoom(PlayerController);
		return;
	}

	const TWeakObjectPtr<APlayerController> WeakPlayerController(PlayerController);
	const FString PreviousRoomCode = A302RoomScope::NormalizeRoomCode(AssignedRoomCodeByController.FindRef(WeakPlayerController));
	if (!PreviousRoomCode.IsEmpty() && !A302RoomScope::MatchesRoomCodeStrict(PreviousRoomCode, NormalizedRoomCode))
	{
		if (TSet<TWeakObjectPtr<APlayerController>>* PreviousRoomPlayers = PlayersByRoomCode.Find(PreviousRoomCode))
		{
			PreviousRoomPlayers->Remove(WeakPlayerController);
			if (PreviousRoomPlayers->Num() == 0)
			{
				PlayersByRoomCode.Remove(PreviousRoomCode);
			}
		}
	}

	AssignedRoomCodeByController.Add(WeakPlayerController, NormalizedRoomCode);
	PlayersByRoomCode.FindOrAdd(NormalizedRoomCode).Add(WeakPlayerController);
}

void URoomMembershipRegistry::UntrackPlayerRoom(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	const TWeakObjectPtr<APlayerController> WeakPlayerController(PlayerController);
	const FString PreviousRoomCode = A302RoomScope::NormalizeRoomCode(AssignedRoomCodeByController.FindRef(WeakPlayerController));
	AssignedRoomCodeByController.Remove(WeakPlayerController);

	if (PreviousRoomCode.IsEmpty())
	{
		return;
	}

	if (TSet<TWeakObjectPtr<APlayerController>>* PreviousRoomPlayers = PlayersByRoomCode.Find(PreviousRoomCode))
	{
		PreviousRoomPlayers->Remove(WeakPlayerController);
		if (PreviousRoomPlayers->Num() == 0)
		{
			PlayersByRoomCode.Remove(PreviousRoomCode);
		}
	}
}
