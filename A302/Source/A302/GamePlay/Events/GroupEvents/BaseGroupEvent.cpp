#include "GamePlay/Events/GroupEvents/BaseGroupEvent.h"

void UBaseGroupEvent::InitializeContext(const UItemDefinition* InRewardDefinition, AActor* InSourceActor)
{
	RewardDefinition = InRewardDefinition;
	SourceActor = InSourceActor;
}
