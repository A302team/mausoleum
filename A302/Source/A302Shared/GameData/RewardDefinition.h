#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameData/RewardTypes.h"
#include "RewardDefinition.generated.h"

class UTexture2D;

UCLASS(BlueprintType, Abstract)
class A302SHARED_API URewardDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward")
    FName ItemId;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward")
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward")
    TObjectPtr<UTexture2D> Icon = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward")
    ERewardCategory RewardCategory = ERewardCategory::BasicItem;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward|Logic")
    TSubclassOf<UObject> RewardLogicClass;

    UClass* ResolveRewardLogicClass() const
    {
        return RewardLogicClass.Get();
    }
};


