#include "Character/Components/QuickSlotComponent.h"

#include "Character/MyCharacter.h"
#include "Engine/Engine.h"
#include "GameData/ItemDefinition.h"
#include "GameData/ItemInstance.h"
#include "GameData/ItemTypes.h"
#include "GamePlay/Items/BaseItem.h"
#include "Interface/UsableItem.h"
#include "Manager/ItemActionFactory.h"

UQuickSlotComponent::UQuickSlotComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UQuickSlotComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeQuickSlots();

	ItemActionFactory = NewObject<UItemActionFactory>(this);
	if (!ItemActionFactory)
	{
		UE_LOG(LogTemp, Error, TEXT("[QuickSlot] Failed to create ItemActionFactory."));
	}

	UpdateQuickSlotSelectionUI();
}

void UQuickSlotComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateAttackRangeDebugState();
}

bool UQuickSlotComponent::TryPickupItemToQuickSlot(AActor* TargetActor)
{
	UItemDefinition* ItemDefinition = nullptr;
	if (!TryGetItemDefinitionFromActor(TargetActor, ItemDefinition) || !ItemDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] Target has no ItemDefinition: %s"), *GetNameSafe(TargetActor));
		LogAndScreenQuickSlotMessage(TEXT("[QuickSlot] Pickup failed: ItemDefinition is missing on actor."), FColor::Orange, 2.0f);
		return false;
	}

	const int32 EmptySlotIndex = FindEmptyQuickSlotIndex();
	if (EmptySlotIndex == INDEX_NONE)
	{
		LogAndScreenQuickSlotMessage(TEXT("[QuickSlot] Slot is full."), FColor::Orange, 2.0f);
		return false;
	}

	if (!BuildQuickSlotLogicForIndex(EmptySlotIndex, ItemDefinition))
	{
		LogAndScreenQuickSlotMessage(TEXT("[QuickSlot] Pickup failed: Item logic build failed."), FColor::Orange, 2.0f);
		return false;
	}

	UpdateQuickSlotNameUI(EmptySlotIndex, ItemDefinition);

	// Keep first pickup usable immediately when no slot has been selected yet.
	if (SelectedSlotIndex == INDEX_NONE)
	{
		SelectedSlotIndex = EmptySlotIndex;
		UpdateQuickSlotSelectionUI();
	}

	const FString PickedName = ItemDefinition->DisplayName.IsEmpty()
		? ItemDefinition->ItemId.ToString()
		: ItemDefinition->DisplayName.ToString();

	LogAndScreenQuickSlotMessage(
		FString::Printf(TEXT("[QuickSlot] Picked '%s' -> Slot %d"), *PickedName, EmptySlotIndex + 1),
		FColor::Green,
		2.0f
	);

	return true;
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

	const UItemDefinition* ItemDefinition = QuickSlotItems[SlotIndex];
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

	UItemDefinition* ItemDefinition = QuickSlotItems[SelectedSlotIndex];
	UItemInstance* ItemInstance = QuickSlotItemInstances[SelectedSlotIndex];
	UBaseItem* ItemLogic = QuickSlotItemLogics[SelectedSlotIndex];

	if (!ItemDefinition || !ItemInstance || !ItemLogic)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] Use failed: selected slot %d has no usable item."), SelectedSlotIndex + 1);
		return false;
	}

	if (!ItemLogic->GetClass()->ImplementsInterface(UUsableItem::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] Use failed: item logic does not implement IUsableItem."));
		return false;
	}

	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		return false;
	}

	FItemTargetData TargetData;
	if (ItemDefinition->UseMode == EItemUseMode::Targeted)
	{
		FVector TargetLocation = FVector::ZeroVector;
		AActor* TargetActor = FindTargetActorForUse(TargetLocation);
		TargetData.TargetActor = TargetActor;
		TargetData.TargetLocation = TargetLocation;

		if (!TargetActor)
		{
			LogAndScreenQuickSlotMessage(TEXT("[QuickSlot] No attack target found."), FColor::Orange, 1.0f);
			return false;
		}
	}

	if (!IUsableItem::Execute_CanUse(ItemLogic, OwnerCharacter, TargetData))
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] CanUse failed for slot %d."), SelectedSlotIndex + 1);
		LogAndScreenQuickSlotMessage(TEXT("[QuickSlot] Target is not attackable now."), FColor::Orange, 1.0f);
		return false;
	}

	const bool bUsed = IUsableItem::Execute_Use(ItemLogic, OwnerCharacter, TargetData);
	if (!bUsed)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] Use failed for slot %d."), SelectedSlotIndex + 1);
		return false;
	}

	OutUsedItemDefinition = ItemDefinition;
	OutUsedSlotIndex = SelectedSlotIndex;

	if (ItemInstance->IsEmpty())
	{
		ClearQuickSlot(SelectedSlotIndex);
	}

	return true;
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
