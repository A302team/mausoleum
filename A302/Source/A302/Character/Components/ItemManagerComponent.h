#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemManagerComponent.generated.h"

class UBaseItem;
class UItemActionFactory;
class UItemDefinition;
class UItemInstance;
class UItemTargetingComponent;
struct FItemTargetData;

DECLARE_MULTICAST_DELEGATE_TwoParams(FDelegateAddItem, int32, const UItemDefinition*);
DECLARE_MULTICAST_DELEGATE_OneParam(FDelegateRemoveItem, int32);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302_API UItemManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UItemManagerComponent();

	virtual void BeginPlay() override;

	void InitializeSlots(int32 SlotCount);
	bool IsValidSlotIndex(int32 SlotIndex) const;
	bool IsSlotEmpty(int32 SlotIndex) const;
	int32 GetSlotCount() const { return ItemDefinitions.Num(); }
	int32 FindFirstEmptySlotIndex() const;
	bool TryAddItemToFirstEmptySlot(UItemDefinition* ItemDefinition, int32 StackCount, int32& OutAddedSlotIndex);

	bool AddItemToSlot(int32 SlotIndex, UItemDefinition* ItemDefinition, int32 StackCount);
	bool RemoveItemFromSlot(int32 SlotIndex);
	int32 RemoveAllItems();
	bool RemoveFirstItemByItemId(const FName& ItemId, int32& OutRemovedSlotIndex);

	UItemDefinition* GetItemDefinitionAtSlot(int32 SlotIndex) const;
	UItemInstance* GetItemInstanceAtSlot(int32 SlotIndex) const;
	UBaseItem* GetItemLogicAtSlot(int32 SlotIndex) const;

	bool TryUseItemAtSlot(
		int32 SlotIndex,
		ACharacter* Instigator,
		UItemDefinition*& OutUsedItemDefinition,
		bool& bOutBecameEmpty
	);

	bool TryAutoUseFirst(
		ACharacter* Instigator,
		const FItemTargetData& TargetData,
		UItemDefinition*& OutUsedItemDefinition,
		int32& OutUsedSlotIndex,
		bool& bOutBecameEmpty
	);

	FDelegateAddItem DelegateAddItem;
	FDelegateRemoveItem DelegateRemoveItem;

private:
	bool IsAutoUseItem(const UItemDefinition* ItemDefinition) const;

	UPROPERTY()
	TArray<TObjectPtr<UItemDefinition>> ItemDefinitions;

	UPROPERTY()
	TArray<TObjectPtr<UItemInstance>> ItemInstances;

	UPROPERTY()
	TArray<TObjectPtr<UBaseItem>> ItemLogics;

	UPROPERTY()
	TObjectPtr<UItemActionFactory> ItemActionFactory = nullptr;

	UPROPERTY()
	TObjectPtr<UItemTargetingComponent> ItemTargetingComponent = nullptr;
};
