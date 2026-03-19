#include "Character/Components/MaliceComponent.h"

#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Character/MyPlayerController.h"
#include "Character/MyCharacter.h"
#include "Character/MyCharacter.h"

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
            OwnerCharacter->OnRequestPublicMaliceAnnouncement.Broadcast(PlayerName, NewMaliceCount);
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
