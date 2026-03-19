#include "Subsystem/A302RoomEventSubsystem.h"
#include "Room/RoomMembershipRegistry.h"
#include "Character/MyPlayerController.h"
#include "GameMode/A302GameMode.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void UA302RoomEventSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UA302RoomEventSubsystem::BroadcastMaliceAnnouncementToRoom(const FString& RoomCode, const FString& PlayerName, int32 MaliceCount)
{
	TArray<APlayerController*> Players;
	GetPlayersInRoom(RoomCode, Players);

	for (APlayerController* PC : Players)
	{
		if (AMyPlayerController* MyPC = Cast<AMyPlayerController>(PC))
		{
			MyPC->Client_ShowPublicMaliceAnnouncement(PlayerName, MaliceCount);
		}
	}
}

void UA302RoomEventSubsystem::GetPlayersInRoom(const FString& RoomCode, TArray<APlayerController*>& OutPlayers) const
{
	UWorld* World = GetWorld();
	if (!World) return;

	AA302GameMode* Gm = Cast<AA302GameMode>(World->GetAuthGameMode());
	if (Gm && Gm->GetRoomMembershipRegistry())
	{
		Gm->GetRoomMembershipRegistry()->GatherPlayersInRoom(World, RoomCode, OutPlayers);
	}
}
