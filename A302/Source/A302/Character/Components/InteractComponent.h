#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractComponent.generated.h"

class AMyCharacter;
class UUserWidget;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302_API UInteractComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	// Called from MyCharacter::OnInteract input binding.
	void HandleInteractInput();

	// Read by quick-slot flow right after HandleInteractInput.
	AActor* GetLastInteractedActor() const { return LastInteractedActor; }

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

	UPROPERTY()
	TObjectPtr<AActor> LastInteractableActor = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> LastInteractedActor = nullptr;

	UPROPERTY()
	TObjectPtr<UUserWidget> InteractionWidgetInstance = nullptr;

	UPROPERTY()
	TObjectPtr<UUserWidget> CrosshairWidgetInstance = nullptr;
};
