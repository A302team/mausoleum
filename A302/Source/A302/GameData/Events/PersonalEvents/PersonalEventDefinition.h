#pragma once

#include "CoreMinimal.h"
#include "GameData/RewardDefinition.h"
#include "PersonalEventDefinition.generated.h"

UCLASS(BlueprintType, Abstract)
class A302_API UPersonalEventDefinition : public URewardDefinition
{
    GENERATED_BODY()

public:
    UPersonalEventDefinition()
    {
        RewardCategory = ERewardCategory::PersonalEvent;
    }
    
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PersonalEvent")
    bool bIsCancelable = true;
};


