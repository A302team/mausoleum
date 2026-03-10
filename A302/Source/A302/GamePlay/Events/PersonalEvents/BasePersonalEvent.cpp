#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"

void UBasePersonalEvent::InitializeContext(const UItemDefinition* InRewardDefinition, AActor* InSourceActor)
{
	RewardDefinition = InRewardDefinition;
	SourceActor = InSourceActor;
}
