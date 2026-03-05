#include "Character/Components/MaliceComponent.h"

UMaliceComponent::UMaliceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
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
