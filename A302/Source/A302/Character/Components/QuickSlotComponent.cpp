#include "Character/Components/QuickSlotComponent.h"

#include "Character/Components/ItemManagerComponent.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "Engine/Engine.h"
#include "GameData/ItemDefinition.h"
#include "GameData/RewardTypes.h"
#include "GameFramework/Controller.h"

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
}

void UQuickSlotComponent::UpdateQuickSlotSelectionUI() const
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		return;
	}

	AController* RawController = OwnerCharacter->GetController();
	AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(RawController);
	if (!MyPlayerController)
	{
		return;
	}

	MyPlayerController->UpdateQuickSlotSelectionVisual(SelectedSlotIndex);
}

void UQuickSlotComponent::UpdateQuickSlotNameUI(int32 SlotIndex, const UItemDefinition* ItemDefinition) const
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		return;
	}

	AController* RawController = OwnerCharacter->GetController();
	AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(RawController);
	if (!MyPlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] UI skipped: PlayerController is not AMyPlayerController."));
		return;
	}

	if (!ItemDefinition)
	{
		MyPlayerController->UpdateQuickSlotItemVisual(SlotIndex, FText::GetEmpty(), nullptr);
		return;
	}

	const FText ItemName = ItemDefinition->DisplayName.IsEmpty()
		? FText::FromName(ItemDefinition->ItemId)
		: ItemDefinition->DisplayName;

	if (!MyPlayerController->UpdateQuickSlotItemVisual(SlotIndex, ItemName, ItemDefinition->Icon))
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] Failed to refresh UI for slot %d."), SlotIndex + 1);
		LogAndScreenQuickSlotMessage(TEXT("[QuickSlot] UI update failed. Check QuickSlot widget names."), FColor::Orange, 2.0f);
	}
}

UQuickSlotComponent::UQuickSlotComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UQuickSlotComponent::BeginPlay()
{
	Super::BeginPlay();

	ItemManagerComponent = GetOwner() ? GetOwner()->FindComponentByClass<UItemManagerComponent>() : nullptr;
	if (!ItemManagerComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("[QuickSlot] ItemManagerComponent is missing on owner."));
	}
	else
	{
		ItemManagerComponent->DelegateAddItem.RemoveAll(this);
		ItemManagerComponent->DelegateRemoveItem.RemoveAll(this);
		ItemManagerComponent->DelegateAddItem.AddUObject(this, &UQuickSlotComponent::NotifyItemAddedToSlot);
		ItemManagerComponent->DelegateRemoveItem.AddUObject(this, &UQuickSlotComponent::NotifyItemRemovedFromSlot);
	}

	InitializeQuickSlots();
	UpdateQuickSlotSelectionUI();
}

void UQuickSlotComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ItemManagerComponent)
	{
		ItemManagerComponent->DelegateAddItem.RemoveAll(this);
		ItemManagerComponent->DelegateRemoveItem.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

bool UQuickSlotComponent::SelectQuickSlotFromAxisValue(float AxisValue)
{
	if (FMath::IsNearlyZero(AxisValue))
	{
		return false;
	}

	const int32 SlotNumberOneBased = FMath::RoundToInt(AxisValue);
	return SelectQuickSlotByNumber(SlotNumberOneBased);
}

bool UQuickSlotComponent::SelectQuickSlotByNumber(int32 SlotNumberOneBased)
{
	const int32 SlotIndex = SlotNumberOneBased - 1;
	if (!IsValidQuickSlotIndex(SlotIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] Invalid slot number: %d"), SlotNumberOneBased);
		return false;
	}

	SelectedSlotIndex = SlotIndex;
	UpdateQuickSlotSelectionUI();

	const UItemManagerComponent* ItemManager = GetItemManager();
	const UItemDefinition* ItemDefinition = ItemManager ? ItemManager->GetItemDefinitionAtSlot(SlotIndex) : nullptr;
	if (ItemDefinition)
	{
		const FString ItemName = ItemDefinition->DisplayName.IsEmpty()
			? ItemDefinition->ItemId.ToString()
			: ItemDefinition->DisplayName.ToString();

		UE_LOG(LogTemp, Log, TEXT("[QuickSlot] Selected Slot %d (%s)"), SlotNumberOneBased, *ItemName);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[QuickSlot] Selected Slot %d (Empty)"), SlotNumberOneBased);
	}

	return true;
}

bool UQuickSlotComponent::TryUseSelectedItem(UItemDefinition*& OutUsedItemDefinition, int32& OutUsedSlotIndex)
{
	OutUsedItemDefinition = nullptr;
	OutUsedSlotIndex = INDEX_NONE;

	if (!IsValidQuickSlotIndex(SelectedSlotIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] Use failed: no selected slot."));
		return false;
	}

	UItemManagerComponent* ItemManager = GetItemManager();
	if (!ItemManager)
	{
		return false;
	}

	UItemDefinition* ItemDefinition = ItemManager->GetItemDefinitionAtSlot(SelectedSlotIndex);
	if (!ItemDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] Use failed: selected slot %d has no usable item."), SelectedSlotIndex + 1);
		return false;
	}

	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		return false;
	}

	bool bBecameEmpty = false;
	if (!ItemManager->TryUseItemAtSlot(SelectedSlotIndex, OwnerCharacter, OutUsedItemDefinition, bBecameEmpty))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[QuickSlot] Use failed for slot %d. Item=%s UseMode=%d"),
			SelectedSlotIndex + 1,
			*GetNameSafe(ItemDefinition),
			static_cast<int32>(ItemDefinition->UseMode)
		);
		return false;
	}

	OutUsedSlotIndex = SelectedSlotIndex;
	return true;
}

bool UQuickSlotComponent::TryAutoUseItem()
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	UItemManagerComponent* ItemManager = GetItemManager();
	if (!OwnerCharacter || !ItemManager)
	{
		return false;
	}

	UItemDefinition* UsedItemDefinition = nullptr;
	int32 UsedSlotIndex = INDEX_NONE;
	bool bBecameEmpty = false;
	const FItemTargetData EmptyTargetData;

	if (!ItemManager->TryAutoUseFirst(OwnerCharacter, EmptyTargetData, UsedItemDefinition, UsedSlotIndex, bBecameEmpty))
	{
		return false;
	}

	const FString ItemName = (UsedItemDefinition && !UsedItemDefinition->DisplayName.IsEmpty())
		? UsedItemDefinition->DisplayName.ToString()
		: GetNameSafe(UsedItemDefinition);

	LogAndScreenQuickSlotMessage(
		FString::Printf(TEXT("[QuickSlot] Auto-used '%s' (Slot %d)"), *ItemName, UsedSlotIndex + 1),
		FColor::Green,
		1.2f
	);

	return true;
}

bool UQuickSlotComponent::RemoveFirstItemByItemId(const FName& ItemId)
{
	UItemManagerComponent* ItemManager = GetItemManager();
	if (!ItemManager)
	{
		return false;
	}

	int32 RemovedSlotIndex = INDEX_NONE;
	if (!ItemManager->RemoveFirstItemByItemId(ItemId, RemovedSlotIndex))
	{
		return false;
	}

	return true;
}

void UQuickSlotComponent::NotifyItemAddedToSlot(int32 SlotIndex, const UItemDefinition* ItemDefinition)
{
	if (!IsValidQuickSlotIndex(SlotIndex))
	{
		return;
	}

	UpdateQuickSlotNameUI(SlotIndex, ItemDefinition);
	if (SelectedSlotIndex == INDEX_NONE)
	{
		SelectedSlotIndex = SlotIndex;
		UpdateQuickSlotSelectionUI();
	}
}

void UQuickSlotComponent::NotifyItemRemovedFromSlot(int32 SlotIndex)
{
	if (!IsValidQuickSlotIndex(SlotIndex))
	{
		return;
	}

	UpdateQuickSlotNameUI(SlotIndex, nullptr);
	UpdateQuickSlotSelectionUI();
}

void UQuickSlotComponent::LogAndScreenQuickSlotMessage(
	const FString& Message,
	const FColor& Color,
	float Duration
) const
{
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, Duration, Color, Message);
	}
}

bool UQuickSlotComponent::TryAddItemByDefinition(UItemDefinition* ItemDefinition)
{
	if (!ItemDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] AddItem failed: ItemDefinition is null."));
		return false;
	}

	if (ItemDefinition->RewardCategory != ERewardCategory::BasicItem)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[QuickSlot] AddItem skipped: non-basic reward. item=%s category=%d"),
			*GetNameSafe(ItemDefinition),
			static_cast<int32>(ItemDefinition->RewardCategory)
		);
		return false;
	}

	UItemManagerComponent* ItemManager = GetItemManager();
	if (!ItemManager)
	{
		return false;
	}

	int32 AddedSlotIndex = INDEX_NONE;
	if (!ItemManager->TryAddItemToFirstEmptySlot(ItemDefinition, PickupStackCount, AddedSlotIndex))
	{
		LogAndScreenQuickSlotMessage(TEXT("[QuickSlot] Slot is full."), FColor::Orange, 2.0f);
		return false;
	}

	const FString PickedName = ItemDefinition->DisplayName.IsEmpty()
		? ItemDefinition->ItemId.ToString()
		: ItemDefinition->DisplayName.ToString();

	LogAndScreenQuickSlotMessage(
		FString::Printf(TEXT("[QuickSlot] Added '%s' -> Slot %d"), *PickedName, AddedSlotIndex + 1),
		FColor::Green,
		2.0f
	);

	return true;
}
