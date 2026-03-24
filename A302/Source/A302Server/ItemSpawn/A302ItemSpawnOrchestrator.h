#pragma once

#include "CoreMinimal.h"
#include "GameMode/A302GameState.h"
#include "UObject/Object.h"
#include "A302ItemSpawnOrchestrator.generated.h"

class ABaseInteractable;
class AItemSpawnArea;
class UGameInstance;
class UItemDefinition;
class UItemSpawnPolicy;
class UStaticMesh;
enum class EInteractType : uint8;

USTRUCT()
struct FPhaseItemSpawnRuntimeState
{
	GENERATED_BODY()

	int32 TotalSpawnedCount = 0;
	TMap<FName, int32> SpawnedCountByItemId;
};

USTRUCT()
struct FSpawnedRoomInteractableRef
{
	GENERATED_BODY()

	TWeakObjectPtr<ABaseInteractable> Actor;
	EGamePhase SpawnedPhase = EGamePhase::Phase0;
};

USTRUCT()
struct FRoomItemSpawnRuntimeState
{
	GENERATED_BODY()

	FString RoomCode;
	bool bLevelReady = false;
	TMap<EGamePhase, FPhaseItemSpawnRuntimeState> PhaseRuntimeByPhase;
	TArray<FSpawnedRoomInteractableRef> SpawnedActors;
};

USTRUCT()
struct FItemSpawnRuntimeEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UItemDefinition> ItemDefinition = nullptr;

	UPROPERTY()
	int32 MinSpawnCount = 0;

	UPROPERTY()
	int32 MaxSpawnCount = -1;

	UPROPERTY()
	float Weight = 1.0f;
};

USTRUCT()
struct FSpawnVisualRuntimeEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UStaticMesh> WorldMesh = nullptr;

	UPROPERTY()
	float Weight = 1.0f;
};

UCLASS(Transient)
class A302SERVER_API UA302ItemSpawnOrchestrator : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(
		UGameInstance* InGameInstance,
		const TSoftObjectPtr<UItemSpawnPolicy>& InDefaultSpawnPolicy,
		int32 InSpawnRetryPerItem,
		float InRoomMatchToleranceRatio,
		bool bInClearPreviousPhaseItems,
		bool bInLogItemSpawns
	);
	void Deinitialize();

	void HandleRoomLevelReady(const FString& RoomCode, EGamePhase CurrentPhase);
	void HandleRoomPhaseChanged(const FString& RoomCode, EGamePhase NewPhase);
	void ResetRoom(const FString& RoomCode);

private:
	const UItemSpawnPolicy* ResolveSpawnPolicy();
	void BuildPhaseRuntimeEntries(
		EGamePhase Phase,
		int32& OutPhaseTotalMaxSpawn,
		float& OutHoldInteractWeight,
		float& OutQTEInteractWeight,
		TSubclassOf<ABaseInteractable>& OutInteractableClass,
		TArray<FItemSpawnRuntimeEntry>& OutItems,
		TArray<FSpawnVisualRuntimeEntry>& OutVisuals
	);
	void TrySpawnForPhase(const FString& RoomCode, EGamePhase Phase);
	bool TrySpawnSingleItem(
		const FString& RoomCode,
		EGamePhase Phase,
		const FItemSpawnRuntimeEntry& Entry,
		const TArray<AItemSpawnArea*>& CandidateAreas,
		TSubclassOf<ABaseInteractable> InteractableClass,
		const TArray<FSpawnVisualRuntimeEntry>& VisualEntries,
		float HoldInteractWeight,
		float QTEInteractWeight,
		FRoomItemSpawnRuntimeState& RoomState
	);
	TArray<AItemSpawnArea*> CollectCandidateAreas(const FString& RoomCode, EGamePhase Phase) const;
	bool IsAreaInRoomSpace(const AActor* AreaActor, const FString& RoomCode) const;
	void CleanupInvalidActorRefs(FRoomItemSpawnRuntimeState& RoomState) const;
	void DestroyRoomActors(FRoomItemSpawnRuntimeState& RoomState) const;
	void DestroyPhaseActors(FRoomItemSpawnRuntimeState& RoomState, EGamePhase PhaseToKeep) const;
	bool IsSpawnableItemDefinition(const UItemDefinition* ItemDefinition) const;
	UStaticMesh* PickWeightedWorldMesh(const TArray<FSpawnVisualRuntimeEntry>& VisualEntries) const;
	EInteractType PickWeightedInteractType(float HoldWeight, float QTEWeight) const;
	static FString NormalizeRoomCode(const FString& RoomCode);
	UWorld* ResolveWorld() const;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<UGameInstance> OwningGameInstance;

	TSoftObjectPtr<UItemSpawnPolicy> DefaultSpawnPolicy;
	int32 SpawnRetryPerItem = 5;
	float RoomMatchToleranceRatio = 0.45f;
	bool bClearPreviousPhaseItems = true;
	bool bLogItemSpawns = true;

	UPROPERTY(Transient)
	TObjectPtr<UItemSpawnPolicy> CachedSpawnPolicy = nullptr;

	TMap<FString, FRoomItemSpawnRuntimeState> RoomRuntimeStates;
};
