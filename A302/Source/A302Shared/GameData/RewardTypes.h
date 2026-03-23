#pragma once

#include "CoreMinimal.h"
#include "RewardTypes.generated.h"

class URewardDefinition;

UENUM(BlueprintType)
enum class ERewardCategory : uint8
{
    BasicItem      UMETA(DisplayName="Basic Item"),
    PersonalEvent  UMETA(DisplayName="Personal Event"),
    GroupEvent     UMETA(DisplayName="Group Event"),
};

UENUM(BlueprintType)
enum class ERewardAssignmentMode : uint8
{
    Manual         UMETA(DisplayName="Manual"),
    RandomFromPool UMETA(DisplayName="Random From Pool"),
};

USTRUCT(BlueprintType)
struct FWeightedRewardDefinitionEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
    TObjectPtr<URewardDefinition> RewardDefinition = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward", meta = (ClampMin = "0.0"))
    float Weight = 1.0f;
};
