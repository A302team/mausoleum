#include "Character/Components/Inventory/ItemManagerComponent.h"

#include "Character/Components/Inventory/ItemTargetingComponent.h"
#include "GameData/Items/ItemDefinition.h"
#include "GameData/Items/ItemInstance.h"
#include "GamePlay/Factories/ItemActionFactory.h"
#include "GamePlay/Items/BaseItem.h"
#include "GamePlay/Items/ItemKnife.h"
#include "GamePlay/Items/ItemShield.h"
#include "GamePlay/Items/ItemCursedSword.h"
#include "Interface/UsableItem.h"

UItemManagerComponent::UItemManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UItemManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	ItemActionFactory = NewObject<UItemActionFactory>(this);
	if (!ItemActionFactory)
	{
		UE_LOG(LogTemp, Error, TEXT("[ItemManager] Failed to create ItemActionFactory."));
	}

	ItemTargetingComponent = GetOwner() ? GetOwner()->FindComponentByClass<UItemTargetingComponent>() : nullptr;
}

void UItemManagerComponent::InitializeSlots(int32 SlotCount)
{
	const int32 SafeCount = FMath::Max(1, SlotCount);
	ItemDefinitions.Init(nullptr, SafeCount);
	ItemInstances.Init(nullptr, SafeCount);
	ItemLogics.Init(nullptr, SafeCount);
}

bool UItemManagerComponent::IsValidSlotIndex(int32 SlotIndex) const
{
	return ItemDefinitions.IsValidIndex(SlotIndex);
}

bool UItemManagerComponent::IsSlotEmpty(int32 SlotIndex) const
{
	return IsValidSlotIndex(SlotIndex) && ItemDefinitions[SlotIndex] == nullptr;
}

int32 UItemManagerComponent::FindFirstEmptySlotIndex() const
{
	for (int32 SlotIndex = 0; SlotIndex < ItemDefinitions.Num(); ++SlotIndex)
	{
		if (ItemDefinitions[SlotIndex] == nullptr)
		{
			return SlotIndex;
		}
	}

	return INDEX_NONE;
}

bool UItemManagerComponent::TryAddItemToFirstEmptySlot(
	UItemDefinition* ItemDefinition,
	int32 StackCount,
	int32& OutAddedSlotIndex
)
{
	OutAddedSlotIndex = INDEX_NONE;
	if (!ItemDefinition)
	{
		return false;
	}

	const int32 EmptySlotIndex = FindFirstEmptySlotIndex();
	if (EmptySlotIndex == INDEX_NONE)
	{
		return false;
	}

	if (!AddItemToSlot(EmptySlotIndex, ItemDefinition, StackCount))
	{
		return false;
	}

	OutAddedSlotIndex = EmptySlotIndex;
	return true;
}

bool UItemManagerComponent::AddItemToSlot(int32 SlotIndex, UItemDefinition* ItemDefinition, int32 StackCount)
{
	if (!IsValidSlotIndex(SlotIndex) || !ItemDefinition)
	{
		return false;
	}

	UClass* LogicClass = ItemDefinition->ResolveRewardLogicClass();
	const bool bIsCursedSwordLogic = LogicClass && LogicClass->IsChildOf(UItemCursedSword::StaticClass());
	if (ItemDefinition->RewardCategory != ERewardCategory::BasicItem && !bIsCursedSwordLogic)
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

	NewInstance->Init(ItemDefinition, FMath::Max(1, StackCount));

	UBaseItem* NewLogic = ItemActionFactory->CreateLogic(this, NewInstance);
	if (!NewLogic)
	{
		return false;
	}

	ItemDefinitions[SlotIndex] = ItemDefinition;
	ItemInstances[SlotIndex] = NewInstance;
	ItemLogics[SlotIndex] = NewLogic;
	DelegateAddItem.Broadcast(SlotIndex, ItemDefinition);
	return true;
}

bool UItemManagerComponent::RemoveItemFromSlot(int32 SlotIndex)
{
	if (!IsValidSlotIndex(SlotIndex))
	{
		return false;
	}

	const bool bHadItem = ItemDefinitions[SlotIndex] != nullptr;
	ItemDefinitions[SlotIndex] = nullptr;
	ItemInstances[SlotIndex] = nullptr;
	ItemLogics[SlotIndex] = nullptr;
	if (bHadItem)
	{
		DelegateRemoveItem.Broadcast(SlotIndex);
	}
	return true;
}

int32 UItemManagerComponent::RemoveAllItems()
{
	int32 RemovedItemCount = 0;

	for (int32 SlotIndex = 0; SlotIndex < ItemDefinitions.Num(); ++SlotIndex)
	{
		if (!ItemDefinitions[SlotIndex])
		{
			continue;
		}

		RemoveItemFromSlot(SlotIndex);
		++RemovedItemCount;
	}

	return RemovedItemCount;
}

bool UItemManagerComponent::RemoveFirstItemByItemId(const FName& ItemId, int32& OutRemovedSlotIndex)
{
	OutRemovedSlotIndex = INDEX_NONE;
	if (ItemId.IsNone())
	{
		return false;
	}

	for (int32 SlotIndex = 0; SlotIndex < ItemDefinitions.Num(); ++SlotIndex)
	{
		const UItemDefinition* ItemDefinition = ItemDefinitions[SlotIndex];
		if (!ItemDefinition || ItemDefinition->ItemId != ItemId)
		{
			continue;
		}

		RemoveItemFromSlot(SlotIndex);
		OutRemovedSlotIndex = SlotIndex;
		return true;
	}

	return false;
}

UItemDefinition* UItemManagerComponent::GetItemDefinitionAtSlot(int32 SlotIndex) const
{
	return IsValidSlotIndex(SlotIndex) ? ItemDefinitions[SlotIndex] : nullptr;
}

UItemInstance* UItemManagerComponent::GetItemInstanceAtSlot(int32 SlotIndex) const
{
	return IsValidSlotIndex(SlotIndex) ? ItemInstances[SlotIndex] : nullptr;
}

UBaseItem* UItemManagerComponent::GetItemLogicAtSlot(int32 SlotIndex) const
{
	return IsValidSlotIndex(SlotIndex) ? ItemLogics[SlotIndex] : nullptr;
}

bool UItemManagerComponent::TryUseItemAtSlot(
	int32 SlotIndex,
	ACharacter* Instigator,
	UItemDefinition*& OutUsedItemDefinition,
	bool& bOutBecameEmpty
)
{
	OutUsedItemDefinition = nullptr;
	bOutBecameEmpty = false;

	if (!IsValidSlotIndex(SlotIndex) || !Instigator)
	{
		return false;
	}

	UItemDefinition* ItemDefinition = ItemDefinitions[SlotIndex];
	UItemInstance* ItemInstance = ItemInstances[SlotIndex];
	UBaseItem* ItemLogic = ItemLogics[SlotIndex];
	if (!ItemDefinition || !ItemInstance || !ItemLogic)
	{
		return false;
	}

	if (IsAutoUseItem(ItemDefinition))
	{
		return false;
	}

	if (!ItemLogic->GetClass()->ImplementsInterface(UUsableItem::StaticClass()))
	{
		return false;
	}

	FItemTargetData TargetData;
	UClass* LogicClass = ItemDefinition->ResolveRewardLogicClass();
	const bool bRequiresTargetActor =
		ItemDefinition->Payload.UseMode == EItemUseMode::Targeted ||
		(LogicClass && LogicClass->IsChildOf(UItemKnife::StaticClass()));
	const bool bSupportsOptionalTarget = ItemDefinition->Payload.UseMode == EItemUseMode::SelfOrTargeted;

	if (bRequiresTargetActor || bSupportsOptionalTarget)
	{
		if (!ItemTargetingComponent && GetOwner())
		{
			ItemTargetingComponent = GetOwner()->FindComponentByClass<UItemTargetingComponent>();
		}
		if (!ItemTargetingComponent)
		{
			return false;
		}
		if (!ItemTargetingComponent->TryBuildTargetDataForUse(ItemDefinition, TargetData, bRequiresTargetActor))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[ItemManager] TryUseItemAtSlot failed: could not acquire target actor. item=%s"),
				*GetNameSafe(ItemDefinition)
			);
			return false;
		}
	}

	if (!IUsableItem::Execute_CanUse(ItemLogic, Instigator, TargetData))
	{
		return false;
	}

	if (!IUsableItem::Execute_Use(ItemLogic, Instigator, TargetData))
	{
		return false;
	}

	OutUsedItemDefinition = ItemDefinition;

	if (ItemInstance->IsEmpty())
	{
		RemoveItemFromSlot(SlotIndex);
		bOutBecameEmpty = true;
	}

	return true;
}

bool UItemManagerComponent::TryAutoUseFirst(
	ACharacter* Instigator,
	const FItemTargetData& TargetData,
	UItemDefinition*& OutUsedItemDefinition,
	int32& OutUsedSlotIndex,
	bool& bOutBecameEmpty
)
{
	OutUsedItemDefinition = nullptr;
	OutUsedSlotIndex = INDEX_NONE;
	bOutBecameEmpty = false;

	if (!Instigator)
	{
		return false;
	}

	for (int32 SlotIndex = 0; SlotIndex < ItemDefinitions.Num(); ++SlotIndex)
	{
		UItemDefinition* ItemDefinition = ItemDefinitions[SlotIndex];
		UItemInstance* ItemInstance = ItemInstances[SlotIndex];
		UBaseItem* ItemLogic = ItemLogics[SlotIndex];
		if (!ItemDefinition || !ItemInstance || !ItemLogic)
		{
			continue;
		}

		if (!IsAutoUseItem(ItemDefinition))
		{
			continue;
		}

		if (!ItemLogic->GetClass()->ImplementsInterface(UUsableItem::StaticClass()))
		{
			continue;
		}

		if (!IUsableItem::Execute_CanUse(ItemLogic, Instigator, TargetData))
		{
			continue;
		}

		if (!IUsableItem::Execute_Use(ItemLogic, Instigator, TargetData))
		{
			continue;
		}

		OutUsedItemDefinition = ItemDefinition;
		OutUsedSlotIndex = SlotIndex;
		if (ItemInstance->IsEmpty())
		{
			RemoveItemFromSlot(SlotIndex);
			bOutBecameEmpty = true;
		}

		return true;
	}

	return false;
}

bool UItemManagerComponent::IsAutoUseItem(const UItemDefinition* ItemDefinition) const
{
	if (!ItemDefinition)
	{
		return false;
	}

	UClass* LogicClass = ItemDefinition->ResolveRewardLogicClass();
	return ItemDefinition->Payload.AutoUse || (LogicClass && LogicClass->IsChildOf(UItemShield::StaticClass()));
}

