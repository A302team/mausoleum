#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameData/RewardTypes.h"
#include "StageRewardPoolDefinition.generated.h"

UCLASS(BlueprintType)
class A302SHARED_API UStageRewardPoolDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	// Stable stage key for spawners or stage managers.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage")
	FName StageId;

	// User-facing label for editor readability.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage")
	FText DisplayName;

	// Weighted pool of reward definition assets used by interactable spawners.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Reward")
	TArray<FWeightedRewardDefinitionEntry> RewardPool;
};
