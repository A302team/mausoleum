#include "Character/Components/QuickSlotComponent.h"

#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "Engine/Engine.h"
#include "GameData/ItemDefinition.h"
#include "GameFramework/Controller.h"
#include "Object/BaseInteractable.h"

namespace
{
	void LogAndScreenQuickSlot(
		const FString& Message,
		const FColor& Color = FColor::Yellow,
		const float Duration = 3.0f
	)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, Duration, Color, Message);
		}
	}
}

UQuickSlotComponent::UQuickSlotComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UQuickSlotComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeQuickSlots();
}

bool UQuickSlotComponent::TryPickupItemToQuickSlot(AActor* TargetActor)
{
	// Moved from old InteractionQuickSlotComponent quick-slot section.
	UItemDefinition* ItemDefinition = nullptr;
	if (!TryGetItemDefinitionFromActor(TargetActor, ItemDefinition) || !ItemDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] Target has no ItemDefinition: %s"), *GetNameSafe(TargetActor));
		LogAndScreenQuickSlot(TEXT("[QuickSlot] Pickup failed: ItemDefinition is missing on actor."), FColor::Orange, 2.0f);
		return false;
	}

	const int32 EmptySlotIndex = FindEmptyQuickSlotIndex();
	if (EmptySlotIndex == INDEX_NONE)
	{
		LogAndScreenQuickSlot(TEXT("[QuickSlot] Slot is full."), FColor::Orange, 2.0f);
		return false;
	}

	QuickSlotItems[EmptySlotIndex] = ItemDefinition;
	UpdateQuickSlotNameUI(EmptySlotIndex, ItemDefinition);

	const FString PickedName = ItemDefinition->DisplayName.IsEmpty()
		? ItemDefinition->ItemId.ToString()
		: ItemDefinition->DisplayName.ToString();

	LogAndScreenQuickSlot(
		FString::Printf(TEXT("[QuickSlot] Picked '%s' -> Slot %d"), *PickedName, EmptySlotIndex + 1),
		FColor::Green,
		2.0f
	);

	return true;
}

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

void UQuickSlotComponent::InitializeQuickSlots()
{
	QuickSlotItems.Init(nullptr, MaxQuickSlotCount);
}

void UQuickSlotComponent::UpdateQuickSlotNameUI(int32 SlotIndex, const UItemDefinition* ItemDefinition) const
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || !ItemDefinition)
	{
		return;
	}

	AController* RawController = OwnerCharacter->GetController();
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[QuickSlot] Update UI try. Controller=%s Class=%s Slot=%d Item=%s"),
		*GetNameSafe(RawController),
		*GetNameSafe(RawController ? RawController->GetClass() : nullptr),
		SlotIndex + 1,
		*ItemDefinition->ItemId.ToString()
	);

	AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(RawController);
	if (!MyPlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] MyPlayerController is null."));
		LogAndScreenQuickSlot(TEXT("[QuickSlot] UI skipped: PlayerController is not AMyPlayerController."), FColor::Orange, 2.0f);
		return;
	}

	const FText ItemName = ItemDefinition->DisplayName.IsEmpty()
		? FText::FromName(ItemDefinition->ItemId)
		: ItemDefinition->DisplayName;

	if (!MyPlayerController->UpdateQuickSlotItemVisual(SlotIndex, ItemName, ItemDefinition->Icon))
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] Failed to refresh UI for slot %d."), SlotIndex + 1);
		LogAndScreenQuickSlot(TEXT("[QuickSlot] UI update failed. Check QuickSlot widget names."), FColor::Orange, 2.0f);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] UI updated for slot %d."), SlotIndex + 1);
	}
}
