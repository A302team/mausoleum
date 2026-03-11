#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "QuickSlotComponent.generated.h"

class AMyCharacter;
class UItemDefinition;
class UItemManagerComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302_API UQuickSlotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UQuickSlotComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool SelectQuickSlotFromAxisValue(float AxisValue);
	bool SelectQuickSlotByNumber(int32 SlotNumberOneBased);
	bool TryUseSelectedItem(UItemDefinition*& OutUsedItemDefinition, int32& OutUsedSlotIndex);
	bool TryAutoUseItem();
	bool RemoveFirstItemByItemId(const FName& ItemId);
	void NotifyItemAddedToSlot(int32 SlotIndex, const UItemDefinition* ItemDefinition);
	void NotifyItemRemovedFromSlot(int32 SlotIndex);
	int32 GetSelectedSlotIndex() const { return SelectedSlotIndex; }

	UFUNCTION(BlueprintCallable, Category = "QuickSlot|Item")
	bool TryAddItemByDefinition(UItemDefinition* ItemDefinition);

private:
	AMyCharacter* GetOwnerCharacter() const;
	UItemManagerComponent* GetItemManager() const;
	bool IsValidQuickSlotIndex(int32 SlotIndex) const;
	void InitializeQuickSlots();
	void UpdateQuickSlotSelectionUI() const;
	void UpdateQuickSlotNameUI(int32 SlotIndex, const UItemDefinition* ItemDefinition) const;
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
