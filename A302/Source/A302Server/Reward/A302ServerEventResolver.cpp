#include "Reward/A302ServerEventResolver.h"

#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameData/Events/GroupEvents/GroupEventConfiscateDefinition.h"
#include "GameData/RewardDefinition.h"
#include "GamePlay/Events/GroupEvents/BaseGroupEvent.h"
#include "GamePlay/Events/GroupEvents/GroupEventConfiscate.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"

bool FA302ServerEventResolver::TryHandlePersonalEventReward(
	ACharacter* InstigatorCharacter,
	AActor* InteractedActor,
	const URewardDefinition* RewardDefinition
)
{
	if (!InstigatorCharacter || !RewardDefinition)
	{
		return false;
	}

	if (!InstigatorCharacter->HasAuthority())
	{
		return false;
	}

	UClass* LogicClass = RewardDefinition->ResolveRewardLogicClass();
	if (!LogicClass || !LogicClass->IsChildOf(UBasePersonalEvent::StaticClass()))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[EventResolver] %s reward logic is empty or not a valid personal event class."),
			*RewardDefinition->GetName()
		);
		return false;
	}

	UBasePersonalEvent* PersonalEvent = NewObject<UBasePersonalEvent>(InstigatorCharacter, LogicClass);
	if (!PersonalEvent)
	{
		return false;
	}

	PersonalEvent->EventID = RewardDefinition->ItemId;
	PersonalEvent->InitializeRuntimeContext(InstigatorCharacter);
	PersonalEvent->InitializeContext(RewardDefinition, InteractedActor);
	PersonalEvent->ExecuteEvent(InstigatorCharacter);
	return true;
}

bool FA302ServerEventResolver::TryHandleGroupEventReward(
	ACharacter* InstigatorCharacter,
	AActor* InteractedActor,
	const URewardDefinition* RewardDefinition
)
{
	if (!InstigatorCharacter || !InstigatorCharacter->HasAuthority() || !RewardDefinition)
	{
		return false;
	}

	UClass* LogicClass = RewardDefinition->ResolveRewardLogicClass();
	UClass* GroupEventClass = nullptr;
	if (LogicClass && LogicClass->IsChildOf(UBaseGroupEvent::StaticClass()))
	{
		GroupEventClass = LogicClass;
	}

	if (!GroupEventClass)
	{
		URewardDefinition* MutableRewardDefinition = const_cast<URewardDefinition*>(RewardDefinition);
		if (Cast<UGroupEventConfiscateDefinition>(MutableRewardDefinition))
		{
			GroupEventClass = UGroupEventConfiscate::StaticClass();
		}
	}

	if (!GroupEventClass || !GroupEventClass->IsChildOf(UBaseGroupEvent::StaticClass()))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[EventResolver] Group event reward failed: invalid logic class. item=%s logic=%s mapped=%s"),
			*GetNameSafe(RewardDefinition),
			*GetNameSafe(LogicClass),
			*GetNameSafe(GroupEventClass)
		);
		return false;
	}

	UObject* GroupEventOuter = InstigatorCharacter->GetWorld()
		? InstigatorCharacter->GetWorld()->GetAuthGameMode()
		: nullptr;
	if (!GroupEventOuter)
	{
		GroupEventOuter = InstigatorCharacter;
	}

	UBaseGroupEvent* GroupEvent = NewObject<UBaseGroupEvent>(GroupEventOuter, GroupEventClass);
	if (!GroupEvent)
	{
		return false;
	}

	GroupEvent->EventID = RewardDefinition->ItemId;
	GroupEvent->InitializeRuntimeContext(InstigatorCharacter);
	GroupEvent->InitializeContext(RewardDefinition, InteractedActor);
	GroupEvent->ExecuteEvent(InstigatorCharacter);
	return true;
}
