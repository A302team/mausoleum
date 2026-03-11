#pragma once

#include "CoreMinimal.h"
#include "GameData/Events/PersonalEvents/PersonalEventDefinition.h"
#include "PersonalEventTimeKnifeDefinition.generated.h"

class UItemDefinition;

USTRUCT(BlueprintType)
struct FPersonalEventTimeKnifePayload
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PersonalEvent|Payload", meta = (ClampMin = "1.0"))
    float TimedKillDuration = 30.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PersonalEvent|Payload")
    TObjectPtr<UItemDefinition> GrantedItemDefinition = nullptr;
};

UCLASS(BlueprintType)
class A302_API UPersonalEventTimeKnifeDefinition : public UPersonalEventDefinition
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PersonalEvent")
    FPersonalEventTimeKnifePayload Payload;
};
