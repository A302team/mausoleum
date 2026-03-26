#include "ItemSpawn/A302ItemSpawnOrchestrator.h"

#include "GameData/RewardDefinition.h"
#include "Components/ShapeComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Object/BaseInteractable.h"
#include "Object/ItemSpawnArea.h"
#include "Room/RoomWorldOffset.h"

namespace
{
	bool TryResolveGroundPointInSpawnArea(
		UWorld* World,
		const AItemSpawnArea* Area,
		const FVector2D& XY,
		FVector& OutSpawnLocation
	)
	{
		if (!World || !Area)
		{
			return false;
		}

		const UShapeComponent* ShapeComp = Area->GetCollisionComponent();
		if (!ShapeComp)
		{
			return false;
		}

		const FVector BoundsOrigin = ShapeComp->Bounds.Origin;
		const FVector BoundsExtent = ShapeComp->Bounds.BoxExtent;
		const float TraceStartZ = BoundsOrigin.Z + BoundsExtent.Z + 100.0f;
		const float TraceEndZ = BoundsOrigin.Z - BoundsExtent.Z - 100.0f;

		const FVector TraceStart(XY.X, XY.Y, TraceStartZ);
		const FVector TraceEnd(XY.X, XY.Y, TraceEndZ);

		TArray<FHitResult> Hits;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ItemSpawnGroundSnap), false);
		QueryParams.AddIgnoredActor(Area);

		if (!World->LineTraceMultiByChannel(Hits, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams) || Hits.Num() == 0)
		{
			return false;
		}

		// 아래쪽(바닥) 표면을 우선 선택해 지붕/천장 스냅을 방지한다.
		bool bFound = false;
		float LowestZ = TNumericLimits<float>::Max();
		FVector ChosenPoint = FVector::ZeroVector;
		for (const FHitResult& Hit : Hits)
		{
			if (!Hit.bBlockingHit)
			{
				continue;
			}

			// 벽/경사면 오검출 방지.
			if (Hit.ImpactNormal.Z < 0.35f)
			{
				continue;
			}

			if (Hit.ImpactPoint.Z < LowestZ)
			{
				LowestZ = Hit.ImpactPoint.Z;
				ChosenPoint = Hit.ImpactPoint;
				bFound = true;
			}
		}

		if (!bFound)
		{
			return false;
		}

		OutSpawnLocation = ChosenPoint + FVector(0.0f, 0.0f, 20.0f);
		return true;
	}
}

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

	URewardDefinition* RewardDefinition = Entry.RewardDefinition;
	if (!IsSpawnableRewardDefinition(RewardDefinition))
	{
		if (bLogItemSpawns)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[ItemSpawn] TrySpawnSingleItem skipped non-spawnable reward. room=%s phase=%d reward=%s"),
				*RoomCode,
				static_cast<int32>(Phase),
				*GetNameSafe(RewardDefinition)
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
				TEXT("[ItemSpawn] TrySpawnSingleItem has no valid SpawnClass. room=%s phase=%d reward=%s"),
				*RoomCode,
				static_cast<int32>(Phase),
				*GetNameSafe(RewardDefinition)
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
		const FVector2D SpawnXY(RawSpawnPoint.X, RawSpawnPoint.Y);
		if (!TryResolveGroundPointInSpawnArea(World, SelectedArea, SpawnXY, SpawnLocation))
		{
			continue;
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
		SpawnedInteractable->SetRewardDefinition(RewardDefinition);
		UGameplayStatics::FinishSpawningActor(SpawnedInteractable, SpawnTransform);

		FSpawnedRoomInteractableRef& Ref = RoomState.SpawnedActors.AddDefaulted_GetRef();
		Ref.Actor = SpawnedInteractable;
		Ref.SpawnedPhase = Phase;

		if (bLogItemSpawns)
		{
			UE_LOG(
				LogTemp,
				Log,
				TEXT("[ItemSpawn] Spawned reward interactable. room=%s phase=%d reward=%s actor=%s location=(%.0f,%.0f,%.0f)"),
				*RoomCode,
				static_cast<int32>(Phase),
				*GetNameSafe(RewardDefinition),
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
			TEXT("[ItemSpawn] Failed to spawn reward after retries. room=%s phase=%d reward=%s retries=%d"),
			*RoomCode,
			static_cast<int32>(Phase),
			*GetNameSafe(RewardDefinition),
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
