#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "A302ServerRewardBridge.generated.h"

class AActor;
class ACharacter;
class URewardDefinition;

UINTERFACE(MinimalAPI)
class UA302ServerRewardBridge : public UInterface
{
	GENERATED_BODY()
};

class A302SHARED_API IA302ServerRewardBridge
{
	GENERATED_BODY()

public:
	virtual bool TryHandlePersonalEventReward(ACharacter* InstigatorCharacter, AActor* InteractedActor, const URewardDefinition* RewardDefinition) = 0;
	virtual bool TryHandleGroupEventReward(ACharacter* InstigatorCharacter, AActor* InteractedActor, const URewardDefinition* RewardDefinition) = 0;
};
