#include "Character/Components/QuickSlotComponent.h"

#include "Character/Components/ItemManagerComponent.h"
#include "Character/MyCharacter.h"
#include "Engine/Engine.h"
#include "GameData/ItemDefinition.h"
#include "GameData/ItemTypes.h"
#include "GamePlay/Items/BaseItem.h"
#include "GamePlay/Items/ItemKnife.h"
#include "GamePlay/Items/ItemShield.h"
#include "Interface/UsableItem.h"

UQuickSlotComponent::UQuickSlotComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UQuickSlotComponent::BeginPlay()
{
	Super::BeginPlay();

	ItemManagerComponent = GetOwner() ? GetOwner()->FindComponentByClass<UItemManagerComponent>() : nullptr;
	if (!ItemManagerComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("[QuickSlot] ItemManagerComponent is missing on owner."));
	}

	InitializeQuickSlots();
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
	UBaseItem* ItemLogic = ItemManager->GetItemLogicAtSlot(SelectedSlotIndex);
	if (!ItemDefinition || !ItemLogic)
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

	UClass* LogicClass = ItemDefinition->ResolveRewardLogicClass();
	const bool bIsShieldLogicClass = LogicClass && LogicClass->IsChildOf(UItemShield::StaticClass());
	const bool bIsKnifeLogicClass = LogicClass && LogicClass->IsChildOf(UItemKnife::StaticClass());

	const bool bAutoUseOnly = ItemDefinition->AutoUse || bIsShieldLogicClass;
	if (bAutoUseOnly)
	{
		LogAndScreenQuickSlotMessage(TEXT("[QuickSlot] This slot is AutoUse only."), FColor::Orange, 1.2f);
		return false;
	}

	FItemTargetData TargetData;
	const bool bNeedsTarget = ItemDefinition->UseMode == EItemUseMode::Targeted || bIsKnifeLogicClass;
	if (bNeedsTarget)
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

	bool bBecameEmpty = false;
	if (!ItemManager->TryUseItemAtSlot(SelectedSlotIndex, OwnerCharacter, TargetData, OutUsedItemDefinition, bBecameEmpty))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[QuickSlot] Use failed for slot %d. Item=%s UseMode=%d Logic=%s"),
			SelectedSlotIndex + 1,
			*GetNameSafe(ItemDefinition),
			static_cast<int32>(ItemDefinition->UseMode),
			*GetNameSafe(LogicClass)
		);
		return false;
	}

	OutUsedSlotIndex = SelectedSlotIndex;
	if (bBecameEmpty)
	{
		NotifyItemRemovedFromSlot(SelectedSlotIndex);
	}

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

	if (bBecameEmpty)
	{
		NotifyItemRemovedFromSlot(UsedSlotIndex);
	}

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

	if (RemovedSlotIndex != INDEX_NONE)
	{
		NotifyItemRemovedFromSlot(RemovedSlotIndex);
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

	const int32 EmptySlotIndex = FindEmptyQuickSlotIndex();
	if (EmptySlotIndex == INDEX_NONE)
	{
		LogAndScreenQuickSlotMessage(TEXT("[QuickSlot] Slot is full."), FColor::Orange, 2.0f);
		return false;
	}

	if (!BuildQuickSlotItemForIndex(EmptySlotIndex, ItemDefinition))
	{
		LogAndScreenQuickSlotMessage(TEXT("[QuickSlot] AddItem failed: item build failed."), FColor::Orange, 2.0f);
		return false;
	}

	UpdateQuickSlotNameUI(EmptySlotIndex, ItemDefinition);

	if (SelectedSlotIndex == INDEX_NONE)
	{
		SelectedSlotIndex = EmptySlotIndex;
		UpdateQuickSlotSelectionUI();
	}

	const FString PickedName = ItemDefinition->DisplayName.IsEmpty()
		? ItemDefinition->ItemId.ToString()
		: ItemDefinition->DisplayName.ToString();

	LogAndScreenQuickSlotMessage(
		FString::Printf(TEXT("[QuickSlot] Added '%s' -> Slot %d"), *PickedName, EmptySlotIndex + 1),
		FColor::Green,
		2.0f
	);

	return true;
}
