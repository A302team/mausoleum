#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"
#include "PersonalEventPurifyingFlame.generated.h"

UCLASS(BlueprintType)
class A302SERVER_API UPersonalEventPurifyingFlame : public UBasePersonalEvent
{
	GENERATED_BODY()

public:
	virtual void ExecuteEvent_Implementation(ACharacter* InstigatorCharacter) override;
	virtual void OnEventResolved(ACharacter* InstigatorCharacter, int32 ChoiceIndex) override;
};
