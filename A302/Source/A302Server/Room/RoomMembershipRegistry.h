#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RoomMembershipRegistry.generated.h"

class APlayerController;
class UWorld;

UCLASS()
class A302SERVER_API URoomMembershipRegistry : public UObject
{
	GENERATED_BODY()

public:
	void RegisterPendingRoomCode(APlayerController* PlayerController, const FString& Options);
	void ClearPendingRoomCode(APlayerController* PlayerController);
	void UnregisterPlayer(APlayerController* PlayerController);
	FString ResolveInitialRoomCode(APlayerController* PlayerController) const;
	void AssignPlayerToRoom(APlayerController* PlayerController);
	FString GetPlayerRoomCode(const APlayerController* PlayerController) const;
	bool IsPlayerInRoom(const APlayerController* PlayerController, const FString& RoomCode) const;
	void GatherPlayersInRoom(UWorld* World, const FString& RoomCode, TArray<APlayerController*>& OutPlayers) const;
	int32 CountPlayersInRoom(UWorld* World, const FString& RoomCode) const;

private:
	void TrackPlayerRoom(APlayerController* PlayerController, const FString& RoomCode);
	void UntrackPlayerRoom(APlayerController* PlayerController);

	UPROPERTY()
	TMap<TObjectPtr<APlayerController>, FString> PendingRoomCodeByController;

	TMap<TWeakObjectPtr<APlayerController>, FString> AssignedRoomCodeByController;
	TMap<FString, TSet<TWeakObjectPtr<APlayerController>>> PlayersByRoomCode;
};
