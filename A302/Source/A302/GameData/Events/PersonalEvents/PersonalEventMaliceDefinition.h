#pragma once

#include "CoreMinimal.h"
#include "GameData/Events/PersonalEvents/PersonalEventDefinition.h"
#include "PersonalEventMaliceDefinition.generated.h"

USTRUCT(BlueprintType)
struct FPersonalEventMalicePayload
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PersonalEvent|Payload", meta = (ClampMin = "1"))
    int32 MaliceAmount = 1;
};

UCLASS(BlueprintType)
class A302_API UPersonalEventMaliceDefinition : public UPersonalEventDefinition
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PersonalEvent")
    FPersonalEventMalicePayload Payload;
};
