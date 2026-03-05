#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "QuickSlotComponent.generated.h"

class AMyCharacter;
class UBaseItem;
class UItemDefinition;
class UItemInstance;
class UItemActionFactory;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302_API UQuickSlotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UQuickSlotComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	bool TryPickupItemToQuickSlot(AActor* TargetActor);
	bool SelectQuickSlotFromAxisValue(float AxisValue);
	bool SelectQuickSlotByNumber(int32 SlotNumberOneBased);
	bool TryUseSelectedItem(UItemDefinition*& OutUsedItemDefinition, int32& OutUsedSlotIndex);
	int32 GetSelectedSlotIndex() const { return SelectedSlotIndex; }

private:
	AMyCharacter* GetOwnerCharacter() const;
	bool TryGetItemDefinitionFromActor(AActor* TargetActor, UItemDefinition*& OutItemDefinition) const;
	int32 FindEmptyQuickSlotIndex() const;
	bool IsValidQuickSlotIndex(int32 SlotIndex) const;
	void InitializeQuickSlots();
	void ClearQuickSlot(int32 SlotIndex);
	void UpdateQuickSlotSelectionUI() const;
	AActor* FindTargetActorForUse(FVector& OutTargetLocation) const;
	bool BuildQuickSlotLogicForIndex(int32 SlotIndex, UItemDefinition* ItemDefinition);
	void UpdateAttackRangeDebugState();
	void UpdateQuickSlotNameUI(int32 SlotIndex, const UItemDefinition* ItemDefinition) const;
	void LogAndScreenQuickSlotMessage(
		const FString& Message,
		const FColor& Color = FColor::Yellow,
		float Duration = 3.0f
	) const;

	UPROPERTY(EditAnywhere, Category = "Item|QuickSlot", meta = (ClampMin = "1"))
	int32 MaxQuickSlotCount = 5;

	UPROPERTY(EditAnywhere, Category = "Item|QuickSlot", meta = (ClampMin = "1"))
	int32 PickupStackCount = 1;

	UPROPERTY(EditAnywhere, Category = "Item|QuickSlot", meta = (ClampMin = "1.0"))
	float UseTraceDistance = 300.f;

	UPROPERTY(VisibleInstanceOnly, Category = "Item|QuickSlot")
	int32 SelectedSlotIndex = INDEX_NONE;

	UPROPERTY(VisibleInstanceOnly, Category = "Item|QuickSlot")
	TArray<TObjectPtr<UItemDefinition>> QuickSlotItems;

	UPROPERTY()
	TArray<TObjectPtr<UItemInstance>> QuickSlotItemInstances;

	UPROPERTY()
	TArray<TObjectPtr<UBaseItem>> QuickSlotItemLogics;

	UPROPERTY()
	TObjectPtr<UItemActionFactory> ItemActionFactory = nullptr;

	bool bWasAttackTargetInRange = false;
};
