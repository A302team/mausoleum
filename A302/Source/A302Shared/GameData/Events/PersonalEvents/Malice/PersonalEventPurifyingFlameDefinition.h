#pragma once

#include "CoreMinimal.h"
#include "GameData/Events/PersonalEvents/PersonalEventDefinition.h"
#include "PersonalEventPurifyingFlameDefinition.generated.h"

class UItemDefinition;

UCLASS(BlueprintType)
class A302SHARED_API UPersonalEventPurifyingFlameDefinition : public UPersonalEventDefinition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PurifyingFlame")
	TObjectPtr<UItemDefinition> SereneLanternDefinition = nullptr;
};
