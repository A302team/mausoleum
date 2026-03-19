#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"
#include "PersonalEventPurifyingFlame.generated.h"

UCLASS(BlueprintType)
class A302_API UPersonalEventPurifyingFlame : public UBasePersonalEvent
{
	GENERATED_BODY()

public:
	virtual void ExecuteEvent_Implementation(AMyCharacter* InstigatorCharacter) override;
	virtual void OnEventResolved_Implementation(AMyCharacter* InstigatorCharacter, int32 ChoiceIndex) override;
};
