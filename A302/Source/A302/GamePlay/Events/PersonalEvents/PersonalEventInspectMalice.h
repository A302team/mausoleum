#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"
#include "PersonalEventInspectMalice.generated.h"

UCLASS(BlueprintType)
class A302_API UPersonalEventInspectMalice : public UBasePersonalEvent
{
    GENERATED_BODY()

public:
    virtual void ExecuteEvent_Implementation(AMyCharacter* InstigatorCharacter) override;
};
