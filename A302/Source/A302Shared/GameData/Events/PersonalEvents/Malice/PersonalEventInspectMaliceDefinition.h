#pragma once

#include "CoreMinimal.h"
#include "GameData/Events/PersonalEvents/PersonalEventDefinition.h"
#include "PersonalEventInspectMaliceDefinition.generated.h"

USTRUCT(BlueprintType)
struct FPersonalEventInspectMalicePayload
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PersonalEvent|Payload", meta = (ClampMin = "0.1"))
    float SelectionTimeoutSeconds = 10.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PersonalEvent|Payload", meta = (ClampMin = "0.1"))
    float ResultDisplaySeconds = 3.0f;
};

UCLASS(BlueprintType)
class A302SHARED_API UPersonalEventInspectMaliceDefinition : public UPersonalEventDefinition
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PersonalEvent")
    FPersonalEventInspectMalicePayload Payload;
};
