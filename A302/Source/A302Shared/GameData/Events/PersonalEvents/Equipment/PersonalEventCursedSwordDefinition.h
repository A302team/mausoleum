#pragma once

#include "CoreMinimal.h"
#include "GameData/Events/PersonalEvents/PersonalEventDefinition.h"
#include "PersonalEventCursedSwordDefinition.generated.h"

class UItemDefinition;

USTRUCT(BlueprintType)
struct FPersonalEventCursedSwordPayload
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PersonalEvent|Payload", meta = (ClampMin = "1.0"))
    float TimedKillDuration = 30.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PersonalEvent|Payload")
    TObjectPtr<UItemDefinition> GrantedItemDefinition = nullptr;
};

UCLASS(BlueprintType)
class A302SHARED_API UPersonalEventCursedSwordDefinition : public UPersonalEventDefinition
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PersonalEvent")
    FPersonalEventCursedSwordPayload Payload;
};
