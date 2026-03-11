#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameData/ItemTypes.h"
#include "ItemDefinition.generated.h"

class UTexture2D;

UCLASS(BlueprintType)
class A302_API UItemDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reward")
    FName ItemId;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reward")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reward")
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reward")
    TObjectPtr<UTexture2D> Icon = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reward")
    ERewardCategory RewardCategory = ERewardCategory::BasicItem;

    // BasicItem/PersonalEvent/GroupEvent logic class.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reward|Logic")
    TSubclassOf<UObject> RewardLogicClass;

    // Basic item parameters
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reward|BasicItem")
    EItemUseMode UseMode = EItemUseMode::SelfCast;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reward|BasicItem")
    bool AutoUse = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reward|BasicItem", meta=(EditCondition="UseMode==EItemUseMode::Targeted"))
    float AttackRange = 200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reward|BasicItem", meta=(EditCondition="UseMode==EItemUseMode::Targeted"))
    bool bRequiresLineOfSight = false;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reward|BasicItem", meta=(EditCondition="UseMode==EItemUseMode::SelfCast"))
    int32 BlockCount = 1;

    // Personal event parameters
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reward|PersonalEvent", meta=(ClampMin="1"))
    int32 MaliceAmount = 1;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reward|PersonalEvent", meta=(ClampMin="1.0"))
    float TimedKillDuration = 30.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Reward|PersonalEvent")
    TObjectPtr<UItemDefinition> GrantedItemDefinition = nullptr;

    UClass* ResolveRewardLogicClass() const
    {
        return RewardLogicClass.Get();
    }
};
