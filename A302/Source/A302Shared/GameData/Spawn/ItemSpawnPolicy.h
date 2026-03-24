#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameMode/A302GameState.h"
#include "ItemSpawnPolicy.generated.h"

class ABaseInteractable;
class UItemDefinition;
class UStaticMesh;

USTRUCT(BlueprintType)
struct FWeightedSpawnVisualEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	TSoftObjectPtr<UStaticMesh> WorldMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;
};

USTRUCT(BlueprintType)
struct FWeightedSpawnLootEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	TSoftObjectPtr<UItemDefinition> ItemDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	int32 MinSpawnCount = 0;

	// -1 means no per-item cap.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "-1"))
	int32 MaxSpawnCount = -1;
};

USTRUCT(BlueprintType)
struct FPhaseItemSpawnPolicy
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Phase")
	EGamePhase Phase = EGamePhase::Phase0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Phase", meta = (ClampMin = "0"))
	int32 PhaseTotalMaxSpawn = 12;

	// If set, this class is used for all spawns in this phase.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn")
	TSubclassOf<ABaseInteractable> InteractableClass;

	// Hold/QTE ratio (1:1 means about half/half).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0"))
	float HoldInteractWeight = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction", meta = (ClampMin = "0.0"))
	float QTEInteractWeight = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	TArray<FWeightedSpawnLootEntry> LootPool;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Visual")
	TArray<FWeightedSpawnVisualEntry> WorldVisualPool;
};

UCLASS(BlueprintType)
class A302SHARED_API UItemSpawnPolicy : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawn")
	TArray<FPhaseItemSpawnPolicy> PhasePolicies;

	const FPhaseItemSpawnPolicy* FindPhasePolicy(EGamePhase Phase) const;
};

