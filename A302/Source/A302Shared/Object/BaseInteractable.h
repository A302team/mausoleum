#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/InteractableInterface.h"
#include "BaseInteractable.generated.h"

class UItemDefinition;
class URewardDefinition;
class UStaticMeshComponent;
class ACharacter;

UCLASS()
class A302SHARED_API ABaseInteractable : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	ABaseInteractable();

	URewardDefinition* GetRewardDefinition() const { return RewardDefinition; }
	UItemDefinition* GetItemDefinition() const; // backward compatibility
	bool TryConsumeInteraction();
	bool IsInteractionConsumed() const { return bInteractionConsumed; }

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void OnInteractionSuccess(ACharacter* PlayerCharacter);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	EInteractType CurrentInteractType = EInteractType::Hold;

	virtual void Interact(ACharacter* PlayerCharacter) override;
	virtual FString GetInteractText() override;
	virtual EInteractType GetInteractType() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh = nullptr;

	// Reward definition consumed by character-side reward routing.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	TObjectPtr<URewardDefinition> RewardDefinition = nullptr;

	UPROPERTY()
	bool bInteractionConsumed = false;
};
