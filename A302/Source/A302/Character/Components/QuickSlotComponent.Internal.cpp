#include "Character/Components/QuickSlotComponent.h"

#include "Character/Components/ItemManagerComponent.h"
#include "Character/MyCharacter.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameData/ItemDefinition.h"
#include "GameData/ItemTypes.h"
#include "GameFramework/Character.h"
#include "GamePlay/Items/BaseItem.h"
#include "Interface/UsableItem.h"
#include "Object/BaseInteractable.h"

AMyCharacter* UQuickSlotComponent::GetOwnerCharacter() const
{
	return Cast<AMyCharacter>(GetOwner());
}

UItemManagerComponent* UQuickSlotComponent::GetItemManager() const
{
	UQuickSlotComponent* MutableThis = const_cast<UQuickSlotComponent*>(this);
	if (!MutableThis->ItemManagerComponent && MutableThis->GetOwner())
	{
		MutableThis->ItemManagerComponent = MutableThis->GetOwner()->FindComponentByClass<UItemManagerComponent>();
	}

	return MutableThis->ItemManagerComponent;
}

bool UQuickSlotComponent::TryGetItemDefinitionFromActor(
	AActor* TargetActor,
	UItemDefinition*& OutItemDefinition
) const
{
	OutItemDefinition = nullptr;

	if (ABaseInteractable* InteractableActor = Cast<ABaseInteractable>(TargetActor))
	{
		OutItemDefinition = InteractableActor->GetRewardDefinition();
	}

	return OutItemDefinition != nullptr;
}

int32 UQuickSlotComponent::FindEmptyQuickSlotIndex() const
{
	const UItemManagerComponent* ItemManager = GetItemManager();
	if (!ItemManager)
	{
		return INDEX_NONE;
	}

	const int32 SlotCount = ItemManager->GetSlotCount();
	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		if (ItemManager->IsSlotEmpty(SlotIndex))
		{
			return SlotIndex;
		}
	}

	return INDEX_NONE;
}

bool UQuickSlotComponent::IsValidQuickSlotIndex(int32 SlotIndex) const
{
	const UItemManagerComponent* ItemManager = GetItemManager();
	return ItemManager && ItemManager->IsValidSlotIndex(SlotIndex);
}

void UQuickSlotComponent::InitializeQuickSlots()
{
	if (UItemManagerComponent* ItemManager = GetItemManager())
	{
		ItemManager->InitializeSlots(MaxQuickSlotCount);
	}

	SelectedSlotIndex = INDEX_NONE;
	bWasAttackTargetInRange = false;
}

void UQuickSlotComponent::ClearQuickSlot(int32 SlotIndex)
{
	if (!IsValidQuickSlotIndex(SlotIndex))
	{
		return;
	}

	if (UItemManagerComponent* ItemManager = GetItemManager())
	{
		ItemManager->RemoveItemFromSlot(SlotIndex);
	}

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

		AActor* HitActor = HitResult.GetActor();
		if (HitActor && HitActor != OwnerCharacter && HitActor->IsA<ACharacter>())
		{
			return HitActor;
		}

		UE_LOG(
			LogTemp,
			Verbose,
			TEXT("[QuickSlot] Trace hit non-character actor: %s. Trying pawn overlap fallback."),
			*GetNameSafe(HitActor)
		);
	}

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

bool UQuickSlotComponent::BuildQuickSlotItemForIndex(int32 SlotIndex, UItemDefinition* ItemDefinition)
{
	if (!IsValidQuickSlotIndex(SlotIndex) || !ItemDefinition)
	{
		return false;
	}

	if (ItemDefinition->RewardCategory != ERewardCategory::BasicItem)
	{
		return false;
	}

	UItemManagerComponent* ItemManager = GetItemManager();
	if (!ItemManager)
	{
		return false;
	}

	return ItemManager->AddItemToSlot(SlotIndex, ItemDefinition, PickupStackCount);
}

void UQuickSlotComponent::UpdateAttackRangeDebugState()
{
	bool bIsTargetInRangeNow = false;
	FString TargetName;

	UItemManagerComponent* ItemManager = GetItemManager();
	if (ItemManager && IsValidQuickSlotIndex(SelectedSlotIndex))
	{
		UItemDefinition* ItemDefinition = ItemManager->GetItemDefinitionAtSlot(SelectedSlotIndex);
		UBaseItem* ItemLogic = ItemManager->GetItemLogicAtSlot(SelectedSlotIndex);
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
