#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "QuickSlotComponent.generated.h"

class AMyCharacter;
class UItemDefinition;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302_API UQuickSlotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UQuickSlotComponent();

	virtual void BeginPlay() override;

	bool TryPickupItemToQuickSlot(AActor* TargetActor);

private:
	AMyCharacter* GetOwnerCharacter() const;
	bool TryGetItemDefinitionFromActor(AActor* TargetActor, UItemDefinition*& OutItemDefinition) const;
	int32 FindEmptyQuickSlotIndex() const;
	void InitializeQuickSlots();
	void UpdateQuickSlotNameUI(int32 SlotIndex, const UItemDefinition* ItemDefinition) const;

	UPROPERTY(EditAnywhere, Category = "Item|QuickSlot", meta = (ClampMin = "1"))
	int32 MaxQuickSlotCount = 5;

	UPROPERTY(VisibleInstanceOnly, Category = "Item|QuickSlot")
	TArray<TObjectPtr<UItemDefinition>> QuickSlotItems;
};
