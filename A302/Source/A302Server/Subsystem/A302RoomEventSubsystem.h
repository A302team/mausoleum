#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "A302RoomEventSubsystem.generated.h"

class APlayerController;
class URoomMembershipRegistry;

/**
 * 방 단위 이벤트 전송을 담당하는 서버 측 서브시스템입니다.
 * RoomMembershipRegistry를 참조하여 특정 방에 있는 플레이어들에게만 RPC를 전파합니다.
 */
UCLASS()
class A302SERVER_API UA302RoomEventSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** 방에 있는 모든 플레이어에게 공지사항을 전송합니다. */
	void BroadcastMaliceAnnouncementToRoom(const FString& RoomCode, const FString& PlayerName, int32 MaliceCount);

	/** 방에 있는 플레이어 리스트를 가져옵니다. */
	void GetPlayersInRoom(const FString& RoomCode, TArray<APlayerController*>& OutPlayers) const;

private:
	UPROPERTY()
	TObjectPtr<URoomMembershipRegistry> RoomRegistry;
};
