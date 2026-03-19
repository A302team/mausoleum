#pragma once

#include "CoreMinimal.h"
#include "GameData/Events/PersonalEvents/PersonalEventDefinition.h"
#include "PersonalEventAnothersPortraitDefinition.generated.h"

class UItemDefinition;

UCLASS(BlueprintType)
class A302SHARED_API UPersonalEventAnothersPortraitDefinition : public UPersonalEventDefinition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AnothersPortrait")
	TObjectPtr<UItemDefinition> OminousMirrorDefinition = nullptr;
};
