#include "ItemSpawn/A302ItemSpawnOrchestrator.h"

#include "GameData/Items/ItemDefinition.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Object/BaseInteractable.h"
#include "Object/ItemSpawnArea.h"
#include "Room/RoomWorldOffset.h"

bool UA302ItemSpawnOrchestrator::TrySpawnSingleItem(
	const FString& RoomCode,
	EGamePhase Phase,
	const FItemSpawnRuntimeEntry& Entry,
	const TArray<AItemSpawnArea*>& CandidateAreas,
	TSubclassOf<ABaseInteractable> InteractableClass,
	const TArray<FSpawnVisualRuntimeEntry>& VisualEntries,
	float HoldInteractWeight,
	float QTEInteractWeight,
	FRoomItemSpawnRuntimeState& RoomState
)
{
	UWorld* World = ResolveWorld();
	if (!World || CandidateAreas.Num() <= 0)
	{
		if (bLogItemSpawns)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[ItemSpawn] TrySpawnSingleItem failed early. room=%s phase=%d hasWorld=%s areaCount=%d"),
				*RoomCode,
				static_cast<int32>(Phase),
				World ? TEXT("true") : TEXT("false"),
				CandidateAreas.Num()
			);
		}
		return false;
	}

	UItemDefinition* ItemDefinition = Entry.ItemDefinition;
	if (!IsSpawnableItemDefinition(ItemDefinition))
	{
		if (bLogItemSpawns)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[ItemSpawn] TrySpawnSingleItem skipped non-spawnable item. room=%s phase=%d item=%s"),
				*RoomCode,
				static_cast<int32>(Phase),
				*GetNameSafe(ItemDefinition)
			);
		}
		return false;
	}

	TSubclassOf<ABaseInteractable> SpawnClass = InteractableClass;
	if (!SpawnClass)
	{
		SpawnClass = ABaseInteractable::StaticClass();
	}
	if (!SpawnClass)
	{
		if (bLogItemSpawns)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[ItemSpawn] TrySpawnSingleItem has no valid SpawnClass. room=%s phase=%d item=%s"),
				*RoomCode,
				static_cast<int32>(Phase),
				*GetNameSafe(ItemDefinition)
			);
		}
		return false;
	}

	const int32 RetryCount = FMath::Max(1, SpawnRetryPerItem);
	for (int32 Attempt = 0; Attempt < RetryCount; ++Attempt)
	{
		float TotalAreaWeight = 0.0f;
		for (const AItemSpawnArea* Area : CandidateAreas)
		{
			if (!IsValid(Area))
			{
				continue;
			}

			TotalAreaWeight += FMath::Max(0.0f, Area->AreaWeight);
		}

		if (TotalAreaWeight <= KINDA_SMALL_NUMBER)
		{
			if (bLogItemSpawns)
			{
				UE_LOG(
					LogTemp,
					Warning,
					TEXT("[ItemSpawn] Area weights are zero. room=%s phase=%d"),
					*RoomCode,
					static_cast<int32>(Phase)
				);
			}
			return false;
		}

		const float Pick = FMath::FRandRange(0.0f, TotalAreaWeight);
		float AccWeight = 0.0f;
		const AItemSpawnArea* SelectedArea = nullptr;

		for (const AItemSpawnArea* Area : CandidateAreas)
		{
			if (!IsValid(Area))
			{
				continue;
			}

			AccWeight += FMath::Max(0.0f, Area->AreaWeight);
			if (Pick <= AccWeight)
			{
				SelectedArea = Area;
				break;
			}
		}

		if (!SelectedArea)
		{
			SelectedArea = CandidateAreas.Last();
		}

		if (!IsValid(SelectedArea))
		{
			continue;
		}

		const FVector RawSpawnPoint = SelectedArea->GetRandomPointInBox();
		FVector SpawnLocation = RawSpawnPoint;

		// Keep random XY in area, but snap Z to nearby walkable/static geometry when possible.
		{
			const FVector TraceStart(RawSpawnPoint.X, RawSpawnPoint.Y, RawSpawnPoint.Z + 3000.0f);
			const FVector TraceEnd(RawSpawnPoint.X, RawSpawnPoint.Y, RawSpawnPoint.Z - 3000.0f);
			FHitResult Hit;
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ItemSpawnGroundSnap), false);
			QueryParams.AddIgnoredActor(SelectedArea);

			if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
			{
				SpawnLocation = Hit.ImpactPoint + FVector(0.0f, 0.0f, 20.0f);
			}
		}

		const FTransform SpawnTransform(
			FRotator(0.0f, FMath::FRandRange(0.0f, 359.0f), 0.0f),
			SpawnLocation
		);

		ABaseInteractable* SpawnedInteractable = World->SpawnActorDeferred<ABaseInteractable>(
			SpawnClass,
			SpawnTransform,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn
		);
		if (!SpawnedInteractable)
		{
			continue;
		}

		if (UStaticMesh* SpawnVisualMesh = PickWeightedWorldMesh(VisualEntries))
		{
			SpawnedInteractable->SetWorldSpawnMesh(SpawnVisualMesh);
		}

		SpawnedInteractable->SetInteractType(PickWeightedInteractType(HoldInteractWeight, QTEInteractWeight));
		SpawnedInteractable->SetRewardDefinition(ItemDefinition);
		UGameplayStatics::FinishSpawningActor(SpawnedInteractable, SpawnTransform);

		FSpawnedRoomInteractableRef& Ref = RoomState.SpawnedActors.AddDefaulted_GetRef();
		Ref.Actor = SpawnedInteractable;
		Ref.SpawnedPhase = Phase;

		if (bLogItemSpawns)
		{
			UE_LOG(
				LogTemp,
				Log,
				TEXT("[ItemSpawn] Spawned item interactable. room=%s phase=%d item=%s actor=%s location=(%.0f,%.0f,%.0f)"),
				*RoomCode,
				static_cast<int32>(Phase),
				*GetNameSafe(ItemDefinition),
				*GetNameSafe(SpawnedInteractable),
				SpawnLocation.X,
				SpawnLocation.Y,
				SpawnLocation.Z
			);
		}

		return true;
	}

	if (bLogItemSpawns)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ItemSpawn] Failed to spawn item after retries. room=%s phase=%d item=%s retries=%d"),
			*RoomCode,
			static_cast<int32>(Phase),
			*GetNameSafe(ItemDefinition),
			RetryCount
		);
	}

	return false;
}

TArray<AItemSpawnArea*> UA302ItemSpawnOrchestrator::CollectCandidateAreas(const FString& RoomCode, EGamePhase Phase) const
{
	TArray<AItemSpawnArea*> OutAreas;

	UWorld* World = ResolveWorld();
	if (!World)
	{
		return OutAreas;
	}

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, AItemSpawnArea::StaticClass(), FoundActors);

	for (AActor* FoundActor : FoundActors)
	{
		AItemSpawnArea* Area = Cast<AItemSpawnArea>(FoundActor);
		if (!IsValid(Area))
		{
			continue;
		}

		if (!Area->IsPhaseAllowed(Phase))
		{
			continue;
		}

		if (!IsAreaInRoomSpace(Area, RoomCode))
		{
			continue;
		}

		OutAreas.Add(Area);
	}

	return OutAreas;
}

bool UA302ItemSpawnOrchestrator::IsAreaInRoomSpace(const AActor* AreaActor, const FString& RoomCode) const
{
	if (!AreaActor)
	{
		return false;
	}

	const FVector RoomOffset = A302RoomWorldOffset::ResolveRoomOffset(RoomCode);
	const double DistanceX = FMath::Abs(AreaActor->GetActorLocation().X - RoomOffset.X);
	const double AcceptRangeX = A302RoomWorldOffset::DefaultOffsetStepX * FMath::Max(0.1f, RoomMatchToleranceRatio);
	return DistanceX <= AcceptRangeX;
}

void UA302ItemSpawnOrchestrator::CleanupInvalidActorRefs(FRoomItemSpawnRuntimeState& RoomState) const
{
	RoomState.SpawnedActors.RemoveAll([](const FSpawnedRoomInteractableRef& Ref)
	{
		return !Ref.Actor.IsValid();
	});
}

void UA302ItemSpawnOrchestrator::DestroyRoomActors(FRoomItemSpawnRuntimeState& RoomState) const
{
	for (const FSpawnedRoomInteractableRef& Ref : RoomState.SpawnedActors)
	{
		if (ABaseInteractable* Interactable = Ref.Actor.Get())
		{
			Interactable->Destroy();
		}
	}

	RoomState.SpawnedActors.Reset();
}

void UA302ItemSpawnOrchestrator::DestroyPhaseActors(FRoomItemSpawnRuntimeState& RoomState, EGamePhase PhaseToKeep) const
{
	for (const FSpawnedRoomInteractableRef& Ref : RoomState.SpawnedActors)
	{
		if (Ref.SpawnedPhase == PhaseToKeep)
		{
			continue;
		}

		if (ABaseInteractable* Interactable = Ref.Actor.Get())
		{
			Interactable->Destroy();
		}
	}

	RoomState.SpawnedActors.RemoveAll([PhaseToKeep](const FSpawnedRoomInteractableRef& Ref)
	{
		return Ref.SpawnedPhase != PhaseToKeep || !Ref.Actor.IsValid();
	});
}

UStaticMesh* UA302ItemSpawnOrchestrator::PickWeightedWorldMesh(const TArray<FSpawnVisualRuntimeEntry>& VisualEntries) const
{
	float TotalWeight = 0.0f;
	for (const FSpawnVisualRuntimeEntry& Entry : VisualEntries)
	{
		if (Entry.WorldMesh && Entry.Weight > 0.0f)
		{
			TotalWeight += Entry.Weight;
		}
	}

	if (TotalWeight <= KINDA_SMALL_NUMBER)
	{
		return nullptr;
	}

	const float Pick = FMath::FRandRange(0.0f, TotalWeight);
	float AccWeight = 0.0f;

	for (const FSpawnVisualRuntimeEntry& Entry : VisualEntries)
	{
		if (!Entry.WorldMesh || Entry.Weight <= 0.0f)
		{
			continue;
		}

		AccWeight += Entry.Weight;
		if (Pick <= AccWeight)
		{
			return Entry.WorldMesh;
		}
	}

	return VisualEntries.Last().WorldMesh;
}

EInteractType UA302ItemSpawnOrchestrator::PickWeightedInteractType(float HoldWeight, float QTEWeight) const
{
	const float SafeHold = FMath::Max(0.0f, HoldWeight);
	const float SafeQTE = FMath::Max(0.0f, QTEWeight);
	const float Total = SafeHold + SafeQTE;

	if (Total <= KINDA_SMALL_NUMBER)
	{
		return EInteractType::Hold;
	}

	const float Pick = FMath::FRandRange(0.0f, Total);
	return (Pick <= SafeHold) ? EInteractType::Hold : EInteractType::QTE;
}
