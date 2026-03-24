#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "QuickSlotComponent.generated.h"

class ACharacter;
class UItemDefinition;
class UItemManagerComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnQuickSlotSelectionChanged, int32);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnQuickSlotItemChanged, int32, const UItemDefinition*);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302SHARED_API UQuickSlotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UQuickSlotComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool SelectQuickSlotFromAxisValue(float AxisValue);
	bool SelectQuickSlotByNumber(int32 SlotNumberOneBased);
	bool TryUseSelectedItem(UItemDefinition*& OutUsedItemDefinition, int32& OutUsedSlotIndex);
	bool TryAutoUseItem(
		UItemDefinition** OutUsedItemDefinition = nullptr,
		int32* OutUsedSlotIndex = nullptr,
		bool* bOutBecameEmpty = nullptr
	);
	bool RemoveFirstItemByItemId(const FName& ItemId);
	void NotifyItemAddedToSlot(int32 SlotIndex, const UItemDefinition* ItemDefinition);
	void NotifyItemRemovedFromSlot(int32 SlotIndex);
	int32 GetSelectedSlotIndex() const { return SelectedSlotIndex; }
	const UItemDefinition* GetItemDefinitionAtSlot(int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, Category = "QuickSlot|Item")
	bool TryAddItemByDefinition(UItemDefinition* ItemDefinition);

	FOnQuickSlotSelectionChanged OnQuickSlotSelectionChanged;
	FOnQuickSlotItemChanged OnQuickSlotItemChanged;

private:
	ACharacter* GetOwnerCharacter() const;
	UItemManagerComponent* GetItemManager() const;
	bool IsValidQuickSlotIndex(int32 SlotIndex) const;
	void InitializeQuickSlots();
	void BroadcastQuickSlotSelectionChanged() const;
	void BroadcastQuickSlotItemChanged(int32 SlotIndex, const UItemDefinition* ItemDefinition) const;
	void LogAndScreenQuickSlotMessage(
		const FString& Message,
		const FColor& Color = FColor::Yellow,
		float Duration = 3.0f
	) const;

	UPROPERTY(EditAnywhere, Category = "Item|QuickSlot", meta = (ClampMin = "1"))
	int32 MaxQuickSlotCount = 6;

	UPROPERTY(EditAnywhere, Category = "Item|QuickSlot", meta = (ClampMin = "1"))
	int32 PickupStackCount = 1;

	UPROPERTY(VisibleInstanceOnly, Category = "Item|QuickSlot")
	int32 SelectedSlotIndex = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<UItemManagerComponent> ItemManagerComponent = nullptr;
};
