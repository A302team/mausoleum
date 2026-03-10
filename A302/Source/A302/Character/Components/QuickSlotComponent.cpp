#include "Character/Components/QuickSlotComponent.h"

#include "Character/MyCharacter.h"
#include "Engine/Engine.h"
#include "GameData/ItemDefinition.h"
#include "GameData/ItemInstance.h"
#include "GameData/ItemTypes.h"
#include "GamePlay/Items/BaseItem.h"
#include "GamePlay/Items/ItemKnife.h"
#include "GamePlay/Items/ItemShield.h"
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

	const bool bIsShieldLogicClass =
		ItemDefinition->ItemLogicClass &&
		ItemDefinition->ItemLogicClass->IsChildOf(UItemShield::StaticClass());
	const bool bIsKnifeLogicClass =
		ItemDefinition->ItemLogicClass &&
		ItemDefinition->ItemLogicClass->IsChildOf(UItemKnife::StaticClass());

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

	if (!IUsableItem::Execute_CanUse(ItemLogic, OwnerCharacter, TargetData))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[QuickSlot] CanUse failed for slot %d. Item=%s UseMode=%d Logic=%s"),
			SelectedSlotIndex + 1,
			*GetNameSafe(ItemDefinition),
			static_cast<int32>(ItemDefinition->UseMode),
			*GetNameSafe(ItemDefinition->ItemLogicClass)
		);
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

bool UQuickSlotComponent::TryAutoUseItem()
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		return false;
	}

	FItemTargetData TargetData;
	bool bFoundAutoCandidate = false;

	for (int32 SlotIndex = 0; SlotIndex < QuickSlotItems.Num(); ++SlotIndex)
	{
		UItemDefinition* ItemDefinition = QuickSlotItems[SlotIndex];
		UItemInstance* ItemInstance = QuickSlotItemInstances.IsValidIndex(SlotIndex)
			? QuickSlotItemInstances[SlotIndex]
			: nullptr;
		UBaseItem* ItemLogic = QuickSlotItemLogics.IsValidIndex(SlotIndex)
			? QuickSlotItemLogics[SlotIndex]
			: nullptr;

		if (!ItemDefinition || !ItemInstance || !ItemLogic)
		{
			continue;
		}

		const bool bCanAutoUse =
			ItemDefinition->AutoUse ||
			(
				ItemDefinition->ItemLogicClass &&
				ItemDefinition->ItemLogicClass->IsChildOf(UItemShield::StaticClass())
			);

		if (!bCanAutoUse)
		{
			continue;
		}

		bFoundAutoCandidate = true;

		if (!ItemLogic->GetClass()->ImplementsInterface(UUsableItem::StaticClass()))
		{
			UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] Auto-use skipped: slot %d logic has no IUsableItem."), SlotIndex + 1);
			continue;
		}

		if (!IUsableItem::Execute_CanUse(ItemLogic, OwnerCharacter, TargetData))
		{
			UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] Auto-use skipped: CanUse false at slot %d."), SlotIndex + 1);
			continue;
		}

		if (!IUsableItem::Execute_Use(ItemLogic, OwnerCharacter, TargetData))
		{
			UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] Auto-use failed: Use false at slot %d."), SlotIndex + 1);
			continue;
		}

		const FString ItemName = ItemDefinition->DisplayName.IsEmpty()
			? ItemDefinition->ItemId.ToString()
			: ItemDefinition->DisplayName.ToString();

		LogAndScreenQuickSlotMessage(
			FString::Printf(TEXT("[QuickSlot] Auto-used '%s' (Slot %d)"), *ItemName, SlotIndex + 1),
			FColor::Green,
			1.2f
		);

		if (ItemInstance->IsEmpty())
		{
			ClearQuickSlot(SlotIndex);
		}

		return true;
	}

	if (!bFoundAutoCandidate)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] Auto-use failed: no auto-usable item in quick slots."));
	}

	return false;
}

bool UQuickSlotComponent::RemoveFirstItemByItemId(const FName& ItemId)
{
	if (ItemId.IsNone())
	{
		return false;
	}

	for (int32 SlotIndex = 0; SlotIndex < QuickSlotItems.Num(); ++SlotIndex)
	{
		const UItemDefinition* ItemDefinition = QuickSlotItems[SlotIndex];
		if (!ItemDefinition)
		{
			continue;
		}

		if (ItemDefinition->ItemId != ItemId)
		{
			continue;
		}

		ClearQuickSlot(SlotIndex);
		return true;
	}

	return false;
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

	// 1. 빈 슬롯 찾기
	const int32 EmptySlotIndex = FindEmptyQuickSlotIndex();
	if (EmptySlotIndex == INDEX_NONE)
	{
		LogAndScreenQuickSlotMessage(TEXT("[QuickSlot] Slot is full."), FColor::Orange, 2.0f);
		return false;
	}

	// 2. 아이템 로직 빌드
	if (!BuildQuickSlotLogicForIndex(EmptySlotIndex, ItemDefinition))
	{
		LogAndScreenQuickSlotMessage(TEXT("[QuickSlot] AddItem failed: Item logic build failed."), FColor::Orange, 2.0f);
		return false;
	}

	// 3. UI 업데이트
	UpdateQuickSlotNameUI(EmptySlotIndex, ItemDefinition);

	// 4. 슬롯 선택 갱신
	if (SelectedSlotIndex == INDEX_NONE)
	{
		SelectedSlotIndex = EmptySlotIndex;
		UpdateQuickSlotSelectionUI();
	}

	// 5. 성공 로그 출력
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