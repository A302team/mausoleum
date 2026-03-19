#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"

void UBasePersonalEvent::InitializeContext(const URewardDefinition* InRewardDefinition, AActor* InSourceActor)
{
	RewardDefinition = InRewardDefinition;
	SourceActor = InSourceActor;
}
