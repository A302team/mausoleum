#include "Character/Components/QuickSlotComponent.h"

#include "Character/MyCharacter.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameData/ItemDefinition.h"
#include "GameData/ItemInstance.h"
#include "GameData/ItemTypes.h"
#include "GameFramework/Character.h"
#include "GamePlay/Items/BaseItem.h"
#include "Interface/UsableItem.h"
#include "Manager/ItemActionFactory.h"
#include "Object/BaseInteractable.h"

AMyCharacter* UQuickSlotComponent::GetOwnerCharacter() const
{
	return Cast<AMyCharacter>(GetOwner());
}

bool UQuickSlotComponent::TryGetItemDefinitionFromActor(
	AActor* TargetActor,
	UItemDefinition*& OutItemDefinition
) const
{
	OutItemDefinition = nullptr;

	if (ABaseInteractable* InteractableActor = Cast<ABaseInteractable>(TargetActor))
	{
		OutItemDefinition = InteractableActor->GetItemDefinition();
	}

	return OutItemDefinition != nullptr;
}

int32 UQuickSlotComponent::FindEmptyQuickSlotIndex() const
{
	for (int32 SlotIndex = 0; SlotIndex < QuickSlotItems.Num(); ++SlotIndex)
	{
		if (QuickSlotItems[SlotIndex] == nullptr)
		{
			return SlotIndex;
		}
	}

	return INDEX_NONE;
}

bool UQuickSlotComponent::IsValidQuickSlotIndex(int32 SlotIndex) const
{
	return QuickSlotItems.IsValidIndex(SlotIndex);
}

void UQuickSlotComponent::InitializeQuickSlots()
{
	QuickSlotItems.Init(nullptr, MaxQuickSlotCount);
	QuickSlotItemInstances.Init(nullptr, MaxQuickSlotCount);
	QuickSlotItemLogics.Init(nullptr, MaxQuickSlotCount);
	SelectedSlotIndex = INDEX_NONE;
	bWasAttackTargetInRange = false;
}

void UQuickSlotComponent::ClearQuickSlot(int32 SlotIndex)
{
	if (!IsValidQuickSlotIndex(SlotIndex))
	{
		return;
	}

	QuickSlotItems[SlotIndex] = nullptr;
	QuickSlotItemInstances[SlotIndex] = nullptr;
	QuickSlotItemLogics[SlotIndex] = nullptr;

	UpdateQuickSlotNameUI(SlotIndex, nullptr);
	UpdateQuickSlotSelectionUI();

	UE_LOG(LogTemp, Log, TEXT("[QuickSlot] Slot %d cleared."), SlotIndex + 1);
}

AActor* UQuickSlotComponent::FindTargetActorForUse(FVector& OutTargetLocation) const
{
	OutTargetLocation = FVector::ZeroVector;

	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		return nullptr;
	}

	const FVector Start = OwnerCharacter->GetPawnViewLocation();
	const FVector ForwardVector = OwnerCharacter->GetViewRotation().Vector();
	const FVector End = Start + (ForwardVector * UseTraceDistance);

	OutTargetLocation = End;

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerCharacter);

	if (!GetWorld())
	{
		return nullptr;
	}

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
	{
		OutTargetLocation = HitResult.ImpactPoint;
		return HitResult.GetActor();
	}

	// Fallback for cases where character mesh/capsule is not hit by visibility trace:
	// find a nearby pawn in front of the player.
	TArray<FOverlapResult> OverlapResults;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	const bool bHasOverlap = GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		OwnerCharacter->GetActorLocation(),
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(UseTraceDistance),
		Params
	);

	if (!bHasOverlap)
	{
		return nullptr;
	}

	AActor* BestTarget = nullptr;
	float BestScore = -FLT_MAX;

	const FVector OwnerForward = OwnerCharacter->GetViewRotation().Vector();
	for (const FOverlapResult& Overlap : OverlapResults)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!Candidate || Candidate == OwnerCharacter)
		{
			continue;
		}

		if (!Candidate->IsA<ACharacter>())
		{
			continue;
		}

		const FVector ToCandidate = Candidate->GetActorLocation() - OwnerCharacter->GetActorLocation();
		const float Distance = ToCandidate.Size();
		if (Distance <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		// Keep target roughly in front of the player.
		const float DotForward = FVector::DotProduct(OwnerForward, ToCandidate / Distance);
		if (DotForward < 0.1f)
		{
			continue;
		}

		const float Score = DotForward * 1000.0f - Distance;
		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}

	if (BestTarget)
	{
		OutTargetLocation = BestTarget->GetActorLocation();
	}

	return BestTarget;
}

bool UQuickSlotComponent::BuildQuickSlotLogicForIndex(int32 SlotIndex, UItemDefinition* ItemDefinition)
{
	if (!IsValidQuickSlotIndex(SlotIndex) || !ItemDefinition)
	{
		return false;
	}

	if (!ItemActionFactory)
	{
		ItemActionFactory = NewObject<UItemActionFactory>(this);
	}
	if (!ItemActionFactory)
	{
		return false;
	}

	UItemInstance* NewInstance = NewObject<UItemInstance>(this);
	if (!NewInstance)
	{
		return false;
	}

	NewInstance->Init(ItemDefinition, FMath::Max(1, PickupStackCount));

	UBaseItem* NewLogic = ItemActionFactory->CreateLogic(this, NewInstance);
	if (!NewLogic)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] Failed to create item logic for %s"), *ItemDefinition->ItemId.ToString());
		return false;
	}

	QuickSlotItems[SlotIndex] = ItemDefinition;
	QuickSlotItemInstances[SlotIndex] = NewInstance;
	QuickSlotItemLogics[SlotIndex] = NewLogic;
	return true;
}

void UQuickSlotComponent::UpdateAttackRangeDebugState()
{
	bool bIsTargetInRangeNow = false;
	FString TargetName;

	if (IsValidQuickSlotIndex(SelectedSlotIndex))
	{
		UItemDefinition* ItemDefinition = QuickSlotItems[SelectedSlotIndex];
		UBaseItem* ItemLogic = QuickSlotItemLogics[SelectedSlotIndex];
		AMyCharacter* OwnerCharacter = GetOwnerCharacter();

		if (
			ItemDefinition &&
			ItemLogic &&
			OwnerCharacter &&
			ItemDefinition->UseMode == EItemUseMode::Targeted &&
			ItemLogic->GetClass()->ImplementsInterface(UUsableItem::StaticClass())
		)
		{
			FVector TargetLocation = FVector::ZeroVector;
			AActor* TargetActor = FindTargetActorForUse(TargetLocation);
			if (TargetActor)
			{
				FItemTargetData TargetData;
				TargetData.TargetActor = TargetActor;
				TargetData.TargetLocation = TargetLocation;

				bIsTargetInRangeNow = IUsableItem::Execute_CanUse(ItemLogic, OwnerCharacter, TargetData);
				TargetName = TargetActor->GetName();
			}
		}
	}

	if (bIsTargetInRangeNow && !bWasAttackTargetInRange)
	{
		LogAndScreenQuickSlotMessage(
			FString::Printf(TEXT("[QuickSlot] Attack target in range: %s"), *TargetName),
			FColor::Green,
			1.2f
		);
	}

	bWasAttackTargetInRange = bIsTargetInRangeNow;
}
