#include "GamePlay/Events/GroupEvents/BaseGroupEvent.h"

void UBaseGroupEvent::InitializeContext(const URewardDefinition* InRewardDefinition, AActor* InSourceActor)
{
	RewardDefinition = InRewardDefinition;
	SourceActor = InSourceActor;
}
