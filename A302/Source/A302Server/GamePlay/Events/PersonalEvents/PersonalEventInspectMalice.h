#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"
#include "PersonalEventInspectMalice.generated.h"

UCLASS(BlueprintType)
class A302SERVER_API UPersonalEventInspectMalice : public UBasePersonalEvent
{
    GENERATED_BODY()

public:
    virtual void ExecuteEvent_Implementation(ACharacter* InstigatorCharacter) override;
};
