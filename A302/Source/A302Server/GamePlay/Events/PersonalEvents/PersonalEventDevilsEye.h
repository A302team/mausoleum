#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"
#include "PersonalEventDevilsEye.generated.h"

UCLASS(BlueprintType)
class A302SERVER_API UPersonalEventDevilsEye : public UBasePersonalEvent
{
	GENERATED_BODY()

public:
	virtual void ExecuteEvent_Implementation(ACharacter* InstigatorCharacter) override;
	virtual void OnEventResolvedMulti(ACharacter* InstigatorCharacter, int32 ChoiceIndex) override;
};
