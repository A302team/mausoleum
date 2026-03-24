#include "ItemSpawn/A302ItemSpawnOrchestrator.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameData/Items/ItemDefinition.h"
#include "GameData/Spawn/ItemSpawnPolicy.h"
#include "GamePlay/Items/BaseItem.h"
#include "GamePlay/Items/ItemCursedSword.h"
#include "Object/BaseInteractable.h"
#include "Room/RoomWorldOffset.h"

namespace
{
	struct FResolvedSpawnEntry
	{
		const FItemSpawnRuntimeEntry* Source = nullptr;
		FName ItemId;
		int32 MinCount = 0;
		int32 MaxCount = 0;
	};

	int32 GetPerItemCap(const FItemSpawnRuntimeEntry& Entry, int32 MinCount)
	{
		if (Entry.MaxSpawnCount < 0)
		{
			return TNumericLimits<int32>::Max();
		}

		return FMath::Max(MinCount, Entry.MaxSpawnCount);
	}
}

void UA302ItemSpawnOrchestrator::Initialize(
	UGameInstance* InGameInstance,
	const TSoftObjectPtr<UItemSpawnPolicy>& InDefaultSpawnPolicy,
	int32 InSpawnRetryPerItem,
	float InRoomMatchToleranceRatio,
	bool bInClearPreviousPhaseItems,
	bool bInLogItemSpawns
)
{
	OwningGameInstance = InGameInstance;
	DefaultSpawnPolicy = InDefaultSpawnPolicy;
	SpawnRetryPerItem = FMath::Max(1, InSpawnRetryPerItem);
	RoomMatchToleranceRatio = FMath::Max(0.1f, InRoomMatchToleranceRatio);
	bClearPreviousPhaseItems = bInClearPreviousPhaseItems;
	bLogItemSpawns = bInLogItemSpawns;
	CachedSpawnPolicy = nullptr;
	RoomRuntimeStates.Reset();
}

void UA302ItemSpawnOrchestrator::Deinitialize()
{
	for (TPair<FString, FRoomItemSpawnRuntimeState>& Pair : RoomRuntimeStates)
	{
		DestroyRoomActors(Pair.Value);
	}

	RoomRuntimeStates.Reset();
	CachedSpawnPolicy = nullptr;
	OwningGameInstance = nullptr;
}

void UA302ItemSpawnOrchestrator::HandleRoomLevelReady(const FString& RoomCode, EGamePhase CurrentPhase)
{
	const FString NormalizedRoomCode = NormalizeRoomCode(RoomCode);
	if (NormalizedRoomCode.IsEmpty() || CurrentPhase == EGamePhase::Ended)
	{
		return;
	}

	FRoomItemSpawnRuntimeState& RoomState = RoomRuntimeStates.FindOrAdd(NormalizedRoomCode);
	RoomState.RoomCode = NormalizedRoomCode;
	RoomState.bLevelReady = true;

	TrySpawnForPhase(NormalizedRoomCode, CurrentPhase);
}

void UA302ItemSpawnOrchestrator::HandleRoomPhaseChanged(const FString& RoomCode, EGamePhase NewPhase)
{
	const FString NormalizedRoomCode = NormalizeRoomCode(RoomCode);
	if (NormalizedRoomCode.IsEmpty())
	{
		return;
	}

	if (NewPhase == EGamePhase::Ended)
	{
		ResetRoom(NormalizedRoomCode);
		return;
	}

	FRoomItemSpawnRuntimeState& RoomState = RoomRuntimeStates.FindOrAdd(NormalizedRoomCode);
	RoomState.RoomCode = NormalizedRoomCode;
	CleanupInvalidActorRefs(RoomState);

	if (!RoomState.bLevelReady)
	{
		if (bLogItemSpawns)
		{
			UE_LOG(
				LogTemp,
				Log,
				TEXT("[ItemSpawn] Phase change received before room level ready. spawn deferred. room=%s phase=%d"),
				*NormalizedRoomCode,
				static_cast<int32>(NewPhase)
			);
		}
		return;
	}

	if (bClearPreviousPhaseItems)
	{
		DestroyPhaseActors(RoomState, NewPhase);
	}

	TrySpawnForPhase(NormalizedRoomCode, NewPhase);
}

void UA302ItemSpawnOrchestrator::ResetRoom(const FString& RoomCode)
{
	const FString NormalizedRoomCode = NormalizeRoomCode(RoomCode);
	if (NormalizedRoomCode.IsEmpty())
	{
		return;
	}

	FRoomItemSpawnRuntimeState RoomState;
	if (!RoomRuntimeStates.RemoveAndCopyValue(NormalizedRoomCode, RoomState))
	{
		return;
	}

	DestroyRoomActors(RoomState);

	if (bLogItemSpawns)
	{
		UE_LOG(LogTemp, Log, TEXT("[ItemSpawn] Reset room item spawn state. room=%s"), *NormalizedRoomCode);
	}
}

const UItemSpawnPolicy* UA302ItemSpawnOrchestrator::ResolveSpawnPolicy()
{
	if (CachedSpawnPolicy)
	{
		return CachedSpawnPolicy;
	}

	if (DefaultSpawnPolicy.IsNull())
	{
		if (bLogItemSpawns)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[ItemSpawn] DefaultSpawnPolicy is not configured. Set [/Script/A302Server.A302ItemSpawnSubsystem] DefaultSpawnPolicy in config.")
			);
		}
		return nullptr;
	}

	CachedSpawnPolicy = DefaultSpawnPolicy.LoadSynchronous();
	if (!CachedSpawnPolicy && bLogItemSpawns)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ItemSpawn] Failed to load DefaultSpawnPolicy asset. path=%s"),
			*DefaultSpawnPolicy.ToString()
		);
	}

	return CachedSpawnPolicy;
}

void UA302ItemSpawnOrchestrator::BuildPhaseRuntimeEntries(
	EGamePhase Phase,
	int32& OutPhaseTotalMaxSpawn,
	float& OutHoldInteractWeight,
	float& OutQTEInteractWeight,
	TSubclassOf<ABaseInteractable>& OutInteractableClass,
	TArray<FItemSpawnRuntimeEntry>& OutItems,
	TArray<FSpawnVisualRuntimeEntry>& OutVisuals
)
{
	OutPhaseTotalMaxSpawn = 0;
	OutHoldInteractWeight = 1.0f;
	OutQTEInteractWeight = 1.0f;
	OutInteractableClass = nullptr;
	OutItems.Reset();
	OutVisuals.Reset();

	const UItemSpawnPolicy* SpawnPolicy = ResolveSpawnPolicy();
	if (!SpawnPolicy)
	{
		if (bLogItemSpawns)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ItemSpawn] Spawn policy unavailable. phase=%d"), static_cast<int32>(Phase));
		}
		return;
	}

	const FPhaseItemSpawnPolicy* PhasePolicy = SpawnPolicy->FindPhasePolicy(Phase);
	if (!PhasePolicy)
	{
		if (bLogItemSpawns)
		{
			TArray<FString> AvailablePhaseTexts;
			AvailablePhaseTexts.Reserve(SpawnPolicy->PhasePolicies.Num());
			for (const FPhaseItemSpawnPolicy& ExistingPolicy : SpawnPolicy->PhasePolicies)
			{
				AvailablePhaseTexts.Add(FString::FromInt(static_cast<int32>(ExistingPolicy.Phase)));
			}

			const FString AvailablePhases = AvailablePhaseTexts.Num() > 0
				? FString::Join(AvailablePhaseTexts, TEXT(","))
				: TEXT("<none>");

			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[ItemSpawn] Phase policy not found. requestedPhase=%d configuredPhases=%s"),
				static_cast<int32>(Phase),
				*AvailablePhases
			);
		}
		return;
	}

	OutPhaseTotalMaxSpawn = FMath::Max(0, PhasePolicy->PhaseTotalMaxSpawn);
	OutHoldInteractWeight = FMath::Max(0.0f, PhasePolicy->HoldInteractWeight);
	OutQTEInteractWeight = FMath::Max(0.0f, PhasePolicy->QTEInteractWeight);
	OutInteractableClass = PhasePolicy->InteractableClass;

	int32 InvalidLootAssetCount = 0;
	int32 NotSpawnableLootCount = 0;
	for (const FWeightedSpawnLootEntry& LootEntry : PhasePolicy->LootPool)
	{
		UItemDefinition* ItemDefinition = LootEntry.ItemDefinition.LoadSynchronous();
		if (!ItemDefinition)
		{
			++InvalidLootAssetCount;
			continue;
		}

		if (!IsSpawnableItemDefinition(ItemDefinition))
		{
			++NotSpawnableLootCount;
			continue;
		}

		FItemSpawnRuntimeEntry& OutEntry = OutItems.AddDefaulted_GetRef();
		OutEntry.ItemDefinition = ItemDefinition;
		OutEntry.MinSpawnCount = FMath::Max(0, LootEntry.MinSpawnCount);
		OutEntry.MaxSpawnCount = LootEntry.MaxSpawnCount;
		OutEntry.Weight = FMath::Max(0.0f, LootEntry.Weight);
	}

	int32 InvalidVisualAssetCount = 0;
	for (const FWeightedSpawnVisualEntry& VisualEntry : PhasePolicy->WorldVisualPool)
	{
		UStaticMesh* WorldMesh = VisualEntry.WorldMesh.LoadSynchronous();
		if (!WorldMesh)
		{
			++InvalidVisualAssetCount;
			continue;
		}

		FSpawnVisualRuntimeEntry& OutEntry = OutVisuals.AddDefaulted_GetRef();
		OutEntry.WorldMesh = WorldMesh;
		OutEntry.Weight = FMath::Max(0.0f, VisualEntry.Weight);
	}

	if (bLogItemSpawns)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[ItemSpawn] Phase policy build summary. phase=%d totalMax=%d lootValid=%d lootInvalidAsset=%d lootNotSpawnable=%d visualValid=%d visualInvalidAsset=%d"),
			static_cast<int32>(Phase),
			OutPhaseTotalMaxSpawn,
			OutItems.Num(),
			InvalidLootAssetCount,
			NotSpawnableLootCount,
			OutVisuals.Num(),
			InvalidVisualAssetCount
		);
	}
}

void UA302ItemSpawnOrchestrator::TrySpawnForPhase(const FString& RoomCode, EGamePhase Phase)
{
	FRoomItemSpawnRuntimeState& RoomState = RoomRuntimeStates.FindOrAdd(RoomCode);
	RoomState.RoomCode = RoomCode;
	if (!RoomState.bLevelReady)
	{
		if (bLogItemSpawns)
		{
			UE_LOG(
				LogTemp,
				Log,
				TEXT("[ItemSpawn] Spawn blocked until room level ready. room=%s phase=%d"),
				*RoomCode,
				static_cast<int32>(Phase)
			);
		}
		return;
	}

	int32 PhaseTotalMaxSpawn = 0;
	float HoldInteractWeight = 1.0f;
	float QTEInteractWeight = 1.0f;
	TSubclassOf<ABaseInteractable> InteractableClass;
	TArray<FItemSpawnRuntimeEntry> EffectiveEntries;
	TArray<FSpawnVisualRuntimeEntry> VisualEntries;

	BuildPhaseRuntimeEntries(
		Phase,
		PhaseTotalMaxSpawn,
		HoldInteractWeight,
		QTEInteractWeight,
		InteractableClass,
		EffectiveEntries,
		VisualEntries
	);

	if (bLogItemSpawns)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[ItemSpawn] Policy resolved. room=%s phase=%d totalMax=%d itemPool=%d visualPool=%d holdW=%.2f qteW=%.2f"),
			*RoomCode,
			static_cast<int32>(Phase),
			PhaseTotalMaxSpawn,
			EffectiveEntries.Num(),
			VisualEntries.Num(),
			HoldInteractWeight,
			QTEInteractWeight
		);
	}

	if (PhaseTotalMaxSpawn <= 0 || EffectiveEntries.Num() <= 0)
	{
		if (bLogItemSpawns)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[ItemSpawn] Spawn skipped by policy. room=%s phase=%d totalMax=%d itemPool=%d"),
				*RoomCode,
				static_cast<int32>(Phase),
				PhaseTotalMaxSpawn,
				EffectiveEntries.Num()
			);
		}
		return;
	}

	TArray<AItemSpawnArea*> CandidateAreas = CollectCandidateAreas(RoomCode, Phase);
	if (CandidateAreas.Num() <= 0)
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("[ItemSpawn] No candidate item spawn area found yet. room=%s phase=%d (will retry on next room/phase event)"),
			*RoomCode,
			static_cast<int32>(Phase)
		);
		return;
	}

	CleanupInvalidActorRefs(RoomState);

	FPhaseItemSpawnRuntimeState& PhaseState = RoomState.PhaseRuntimeByPhase.FindOrAdd(Phase);
	if (PhaseState.TotalSpawnedCount >= PhaseTotalMaxSpawn)
	{
		return;
	}

	TArray<FResolvedSpawnEntry> ResolvedEntries;
	ResolvedEntries.Reserve(EffectiveEntries.Num());

	int32 TotalMinRequired = 0;
	for (const FItemSpawnRuntimeEntry& Entry : EffectiveEntries)
	{
		const UItemDefinition* ItemDefinition = Entry.ItemDefinition;
		if (!IsSpawnableItemDefinition(ItemDefinition))
		{
			continue;
		}

		const int32 MinCount = FMath::Max(0, Entry.MinSpawnCount);
		const int32 MaxCount = GetPerItemCap(Entry, MinCount);
		if (MaxCount <= 0)
		{
			continue;
		}

		const FName ItemId = ItemDefinition->ItemId.IsNone()
			? FName(*ItemDefinition->GetPathName())
			: ItemDefinition->ItemId;

		FResolvedSpawnEntry& Resolved = ResolvedEntries.AddDefaulted_GetRef();
		Resolved.Source = &Entry;
		Resolved.ItemId = ItemId;
		Resolved.MinCount = MinCount;
		Resolved.MaxCount = MaxCount;
		TotalMinRequired += MinCount;
	}

	if (ResolvedEntries.Num() <= 0)
	{
		return;
	}

	if (TotalMinRequired > PhaseTotalMaxSpawn)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ItemSpawn] Invalid policy: sum(min) exceeds phase max. room=%s phase=%d minTotal=%d maxTotal=%d"),
			*RoomCode,
			static_cast<int32>(Phase),
			TotalMinRequired,
			PhaseTotalMaxSpawn
		);
	}

	for (const FResolvedSpawnEntry& Entry : ResolvedEntries)
	{
		int32& CurrentItemSpawnedCount = PhaseState.SpawnedCountByItemId.FindOrAdd(Entry.ItemId);
		const int32 RequiredMin = FMath::Min(Entry.MinCount, Entry.MaxCount);
		int32 Missing = FMath::Max(0, RequiredMin - CurrentItemSpawnedCount);

		while (Missing > 0 && PhaseState.TotalSpawnedCount < PhaseTotalMaxSpawn)
		{
			if (!TrySpawnSingleItem(
				RoomCode,
				Phase,
				*Entry.Source,
				CandidateAreas,
				InteractableClass,
				VisualEntries,
				HoldInteractWeight,
				QTEInteractWeight,
				RoomState
			))
			{
				break;
			}

			++CurrentItemSpawnedCount;
			++PhaseState.TotalSpawnedCount;
			--Missing;
		}
	}

	int32 SafeGuard = (PhaseTotalMaxSpawn * 4) + 16;
	while (PhaseState.TotalSpawnedCount < PhaseTotalMaxSpawn && SafeGuard-- > 0)
	{
		float TotalWeight = 0.0f;
		TArray<int32> CandidateIndices;
		CandidateIndices.Reserve(ResolvedEntries.Num());

		for (int32 Index = 0; Index < ResolvedEntries.Num(); ++Index)
		{
			const FResolvedSpawnEntry& Entry = ResolvedEntries[Index];
			const int32 SpawnedCount = PhaseState.SpawnedCountByItemId.FindRef(Entry.ItemId);
			if (SpawnedCount >= Entry.MaxCount)
			{
				continue;
			}

			const float Weight = Entry.Source ? FMath::Max(0.0f, Entry.Source->Weight) : 0.0f;
			if (Weight <= KINDA_SMALL_NUMBER)
			{
				continue;
			}

			TotalWeight += Weight;
			CandidateIndices.Add(Index);
		}

		if (CandidateIndices.Num() <= 0 || TotalWeight <= KINDA_SMALL_NUMBER)
		{
			break;
		}

		const float Pick = FMath::FRandRange(0.0f, TotalWeight);
		float AccWeight = 0.0f;
		int32 PickedIndex = CandidateIndices.Last();

		for (int32 CandidateIndex : CandidateIndices)
		{
			const float Weight = FMath::Max(0.0f, ResolvedEntries[CandidateIndex].Source->Weight);
			AccWeight += Weight;
			if (Pick <= AccWeight)
			{
				PickedIndex = CandidateIndex;
				break;
			}
		}

		const FResolvedSpawnEntry& PickedEntry = ResolvedEntries[PickedIndex];
		if (!TrySpawnSingleItem(
			RoomCode,
			Phase,
			*PickedEntry.Source,
			CandidateAreas,
			InteractableClass,
			VisualEntries,
			HoldInteractWeight,
			QTEInteractWeight,
			RoomState
		))
		{
			continue;
		}

		++PhaseState.SpawnedCountByItemId.FindOrAdd(PickedEntry.ItemId);
		++PhaseState.TotalSpawnedCount;
	}
}

bool UA302ItemSpawnOrchestrator::IsSpawnableItemDefinition(const UItemDefinition* ItemDefinition) const
{
	if (!ItemDefinition)
	{
		return false;
	}

	UClass* LogicClass = ItemDefinition->ResolveRewardLogicClass();
	const bool bIsCursedSwordLogic = LogicClass && LogicClass->IsChildOf(UItemCursedSword::StaticClass());
	if (ItemDefinition->RewardCategory != ERewardCategory::BasicItem && !bIsCursedSwordLogic)
	{
		return false;
	}

	return LogicClass && LogicClass->IsChildOf(UBaseItem::StaticClass());
}

FString UA302ItemSpawnOrchestrator::NormalizeRoomCode(const FString& RoomCode)
{
	return A302RoomWorldOffset::NormalizeRoomCode(RoomCode);
}

UWorld* UA302ItemSpawnOrchestrator::ResolveWorld() const
{
	return OwningGameInstance.IsValid() ? OwningGameInstance->GetWorld() : nullptr;
}
