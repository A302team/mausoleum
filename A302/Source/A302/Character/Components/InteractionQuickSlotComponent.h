#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionQuickSlotComponent.generated.h"

class AMyCharacter;
class UItemDefinition;
class UUserWidget;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302_API UInteractionQuickSlotComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionQuickSlotComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	void HandleInteractInput();

	UPROPERTY(EditAnywhere, Category = "Interaction")
	float InteractionDistance = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> InteractionWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> CrosshairWidgetClass;

private:
	AMyCharacter* GetOwnerCharacter() const;
	void CheckForInteractables();
	void ToggleHighlight(AActor* TargetActor, bool bIsOn) const;
	void HandleInteractOnActor(AActor* TargetActor);
	bool TryPickupItemToQuickSlot(AActor* TargetActor);
	bool TryGetItemDefinitionFromActor(AActor* TargetActor, UItemDefinition*& OutItemDefinition) const;
	int32 FindEmptyQuickSlotIndex() const;
	void InitializeQuickSlots();
	void UpdateQuickSlotNameUI(int32 SlotIndex, const UItemDefinition* ItemDefinition) const;

	UPROPERTY()
	TObjectPtr<AActor> LastInteractableActor = nullptr;

	UPROPERTY()
	TObjectPtr<UUserWidget> InteractionWidgetInstance = nullptr;

	UPROPERTY()
	TObjectPtr<UUserWidget> CrosshairWidgetInstance = nullptr;

	UPROPERTY(VisibleInstanceOnly, Category = "Item|QuickSlot")
	TArray<TObjectPtr<UItemDefinition>> QuickSlotItems;
};
