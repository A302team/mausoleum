#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"
#include "PersonalEventAnothersPortrait.generated.h"

UCLASS(BlueprintType)
class A302SERVER_API UPersonalEventAnothersPortrait : public UBasePersonalEvent
{
	GENERATED_BODY()

public:
	virtual void ExecuteEvent_Implementation(ACharacter* InstigatorCharacter) override;
	virtual void OnEventResolved_Implementation(ACharacter* InstigatorCharacter, int32 ChoiceIndex) override;
};
