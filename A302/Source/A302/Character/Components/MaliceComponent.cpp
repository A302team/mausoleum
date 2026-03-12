#include "Character/Components/MaliceComponent.h"

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

void UMaliceComponent::AddMalice(int32 Count)
{
	if (Count <= 0)
	{
		return;
	}

	MaliceCount = FMath::Max(0, MaliceCount + Count);
	OnMaliceChanged.Broadcast(MaliceCount);
}

void UMaliceComponent::ConsumeMalice(int32 Count)
{
	if (Count <= 0)
	{
		return;
	}

	MaliceCount = FMath::Max(0, MaliceCount - Count);
	OnMaliceChanged.Broadcast(MaliceCount);
}

void UMaliceComponent::OnRep_MaliceCount()
{
	OnMaliceChanged.Broadcast(MaliceCount);
}
