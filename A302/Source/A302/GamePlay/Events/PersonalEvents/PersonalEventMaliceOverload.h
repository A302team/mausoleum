#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"
#include "PersonalEventMaliceOverload.generated.h"

UCLASS()
class A302_API UPersonalEventMaliceOverload : public UBasePersonalEvent
{
	GENERATED_BODY()

public:
	virtual void ExecuteEvent_Implementation(class AMyCharacter* InstigatorCharacter) override;
	virtual void OnEventResolved_Implementation(class AMyCharacter* InstigatorCharacter, int32 ChoiceIndex) override;
};