#pragma once

#include "CoreMinimal.h"
#include "PersonalEventDefinition.h" 
#include "PersonalEventMaliceOverloadDefinition.generated.h"

UCLASS(BlueprintType, Blueprintable)
class A302_API UPersonalEventMaliceOverloadDefinition : public UPersonalEventDefinition
{
	GENERATED_BODY()

public:
	// UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event Data")
	// int32 MaliceIncreaseAmount = 1;
};