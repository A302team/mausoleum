#include "Character/Components/MaliceComponent.h"

#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Character/MyPlayerController.h"
#include "Character/MyCharacter.h"
#include "Character/MyCharacter.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Room/RoomScopeRules.h"

UMaliceComponent::UMaliceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMaliceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UMaliceComponent, MaliceCount);
}

void UMaliceComponent::OnRep_MaliceCount()
{
	OnMaliceChanged.Broadcast(MaliceCount);
}

void UMaliceComponent::BroadcastPublicMaliceAnnouncement(const FString& PlayerName, int32 NewMaliceCount)
{
	if (GetOwner()->HasAuthority())
	{
		if (AMyCharacter* OwnerCharacter = Cast<AMyCharacter>(GetOwner()))
		{
			if (OwnerCharacter->OnRequestPublicMaliceAnnouncement.IsBound())
			{
				OwnerCharacter->OnRequestPublicMaliceAnnouncement.Broadcast(PlayerName, NewMaliceCount);
				return;
			}

			// Fallback: if the server-side bridge delegate is not bound, broadcast directly
			// to players in the same logical room so local/listen tests still receive updates.
			const FString SourceRoomCode = A302RoomScope::ResolvePlayerRoomCode(OwnerCharacter->GetPlayerState());
			if (UWorld* World = OwnerCharacter->GetWorld())
			{
				for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
				{
					AMyPlayerController* TargetController = Cast<AMyPlayerController>(It->Get());
					if (!TargetController)
					{
						continue;
					}

					if (!SourceRoomCode.IsEmpty())
					{
						const FString TargetRoomCode = A302RoomScope::ResolvePlayerRoomCode(TargetController->PlayerState);
						if (!A302RoomScope::MatchesRoomCodeStrict(SourceRoomCode, TargetRoomCode))
						{
							continue;
						}
					}

					TargetController->Client_ShowPublicMaliceAnnouncement(PlayerName, NewMaliceCount);
				}
			}

			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[PublicMalice] Used direct server fallback broadcast. owner=%s room=%s"),
				*GetNameSafe(OwnerCharacter),
				*SourceRoomCode
			);
		}
		return;
	}

	if (AMyCharacter* OwnerCharacter = Cast<AMyCharacter>(GetOwner()))
	{
		if (AMyPlayerController* ClientEventBridge = Cast<AMyPlayerController>(OwnerCharacter->GetController()))
		{
			ClientEventBridge->ShowPublicMaliceAnnouncement(PlayerName, NewMaliceCount);
		}
	}
}

// Multicast_ShowPublicMaliceAnnouncement was replaced by UA302RoomEventSubsystem targeted RPCs.

void UMaliceComponent::AddMalice(int32 Count)
{
	if (Count <= 0 || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	MaliceCount = FMath::Max(0, MaliceCount + Count);
	OnMaliceChanged.Broadcast(MaliceCount);
}

void UMaliceComponent::ConsumeMalice(int32 Count)
{
	if (Count <= 0 || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	MaliceCount = FMath::Max(0, MaliceCount - Count);
	OnMaliceChanged.Broadcast(MaliceCount);
}

void UMaliceComponent::SetRawMalice(int32 NewRawMalice)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	MaliceCount = FMath::Max(0, NewRawMalice + ItemizeMalice);
	OnMaliceChanged.Broadcast(MaliceCount);
}
