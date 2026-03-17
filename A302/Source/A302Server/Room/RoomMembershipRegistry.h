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
	FString ResolveInitialRoomCode(APlayerController* PlayerController) const;
	void AssignPlayerToRoom(APlayerController* PlayerController);
	FString GetPlayerRoomCode(const APlayerController* PlayerController) const;
	bool IsPlayerInRoom(const APlayerController* PlayerController, const FString& RoomCode) const;
	void GatherPlayersInRoom(UWorld* World, const FString& RoomCode, TArray<APlayerController*>& OutPlayers) const;
	int32 CountPlayersInRoom(UWorld* World, const FString& RoomCode) const;

private:
	UPROPERTY()
	TMap<TObjectPtr<APlayerController>, FString> PendingRoomCodeByController;
};
