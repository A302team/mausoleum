#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"
#include "PersonalEventBizarreForge.generated.h"

UCLASS(BlueprintType)
class A302_API UPersonalEventBizarreForge : public UBasePersonalEvent
{
	GENERATED_BODY()

public:
	virtual void ExecuteEvent_Implementation(class AMyCharacter* InstigatorCharacter) override;
	virtual void OnEventResolved_Implementation(class AMyCharacter* InstigatorCharacter, int32 ChoiceIndex) override;

private:
	bool bIsMaliceZeroBranch = false;
};