#pragma once

#include "CoreMinimal.h"

class AActor;
class ACharacter;
class URewardDefinition;

class A302SERVER_API FA302ServerEventResolver
{
public:
	static bool TryHandlePersonalEventReward(ACharacter* InstigatorCharacter, AActor* InteractedActor, const URewardDefinition* RewardDefinition);
	static bool TryHandleGroupEventReward(ACharacter* InstigatorCharacter, AActor* InteractedActor, const URewardDefinition* RewardDefinition);
};
