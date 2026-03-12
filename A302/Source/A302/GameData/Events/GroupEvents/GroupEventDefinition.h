#pragma once

#include "CoreMinimal.h"
#include "GameData/RewardDefinition.h"
#include "GroupEventDefinition.generated.h"

UCLASS(BlueprintType, Abstract)
class A302_API UGroupEventDefinition : public URewardDefinition
{
	GENERATED_BODY()

public:
	UGroupEventDefinition()
	{
		RewardCategory = ERewardCategory::GroupEvent;
	}
};
