#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"
#include "PersonalEventMaliceOverload.generated.h"

class ACharacter;

UCLASS(BlueprintType)
class A302SERVER_API UPersonalEventMaliceOverload : public UBasePersonalEvent
{
	GENERATED_BODY()

public:
	virtual void ExecuteEvent_Implementation(ACharacter* InstigatorCharacter) override;
	virtual void OnEventResolvedMulti(ACharacter* InstigatorCharacter, int32 ChoiceIndex) override;
};
