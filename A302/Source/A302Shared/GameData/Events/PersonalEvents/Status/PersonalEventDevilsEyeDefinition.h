#pragma once

#include "CoreMinimal.h"
#include "GameData/Events/PersonalEvents/PersonalEventDefinition.h"
#include "PersonalEventDevilsEyeDefinition.generated.h"

class UItemDefinition;

UCLASS(BlueprintType)
class A302SHARED_API UPersonalEventDevilsEyeDefinition : public UPersonalEventDefinition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "DevilsEye")
	TObjectPtr<UItemDefinition> CrimsonQuartzDefinition = nullptr;
};
