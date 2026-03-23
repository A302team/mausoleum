#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"
#include "PersonalEventPhase1Collect.generated.h"

UCLASS(BlueprintType)
class A302SERVER_API UPersonalEventPhase1Collect : public UBasePersonalEvent
{
	GENERATED_BODY()

public:
	virtual void ExecuteEvent_Implementation(ACharacter* InstigatorCharacter) override;
};
