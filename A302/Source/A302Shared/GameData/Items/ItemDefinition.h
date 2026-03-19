#pragma once

#include "CoreMinimal.h"
#include "GameData/Items/ItemTypes.h"
#include "GameData/RewardDefinition.h"
#include "ItemDefinition.generated.h"

USTRUCT(BlueprintType)
struct FItemDefinitionPayload
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    EItemUseMode UseMode = EItemUseMode::SelfCast;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1.0", EditCondition = "UseMode==EItemUseMode::Targeted"))
    float ItemUseRange = 200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (EditCondition = "UseMode==EItemUseMode::Targeted"))
    bool bRequiresLineOfSight = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1", EditCondition = "UseMode==EItemUseMode::SelfCast"))
    int32 BlockCount = 1;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
    bool AutoUse = false;
};

UCLASS(BlueprintType)
class A302SHARED_API UItemDefinition : public URewardDefinition
{
    GENERATED_BODY()

public:
    UItemDefinition()
    {
        RewardCategory = ERewardCategory::BasicItem;
    }

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward|Item")
    FItemDefinitionPayload Payload;
};


