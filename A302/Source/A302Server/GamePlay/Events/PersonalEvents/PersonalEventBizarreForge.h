#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"
#include "PersonalEventBizarreForge.generated.h"

UCLASS(BlueprintType)
class A302SERVER_API UPersonalEventBizarreForge : public UBasePersonalEvent
{
	GENERATED_BODY()

public:
	virtual void ExecuteEvent_Implementation(class ACharacter* InstigatorCharacter) override;
	virtual void OnEventResolvedMulti(class ACharacter* InstigatorCharacter, int32 ChoiceIndex) override;

private:
	bool bIsMaliceZeroBranch = false;
};
