#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameData/RewardTypes.h"
#include "Interface/InteractableInterface.h"
#include "BaseInteractable.generated.h"

class UItemDefinition;
class URewardDefinition;
class UStageRewardPoolDefinition;
class UStaticMesh;
class UStaticMeshComponent;
class ACharacter;
class FLifetimeProperty;

UCLASS()
class A302SHARED_API ABaseInteractable : public AActor, public IInteractableInterface
{
	GENERATED_BODY()

public:
	ABaseInteractable();

	URewardDefinition* GetRewardDefinition() const { return RewardDefinition; }
	UItemDefinition* GetItemDefinition() const; // backward compatibility
	void SetRewardDefinition(URewardDefinition* InRewardDefinition);
	void SetInteractType(EInteractType InInteractType);
	void SetWorldSpawnMesh(UStaticMesh* InWorldSpawnMesh);
	bool TryConsumeInteraction();
	void RestoreInteraction();
	bool IsInteractionConsumed() const { return bInteractionConsumed; }

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void OnInteractionSuccess(ACharacter* PlayerCharacter);

	UFUNCTION(BlueprintCallable, Category = "Reward")
	void ApplyStageRewardPoolDefinition(UStageRewardPoolDefinition* StageRewardPoolDefinition);

protected:
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	EInteractType CurrentInteractType = EInteractType::Hold;

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void Interact(ACharacter* PlayerCharacter) override;
	virtual FString GetInteractText() override;
	virtual EInteractType GetInteractType() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	ERewardAssignmentMode RewardAssignmentMode = ERewardAssignmentMode::RandomFromPool;

	// Reward definition consumed by character-side reward routing.
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Reward",
		meta = (EditCondition = "RewardAssignmentMode == ERewardAssignmentMode::Manual", EditConditionHides))
	TObjectPtr<URewardDefinition> RewardDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward",
		meta = (EditCondition = "RewardAssignmentMode == ERewardAssignmentMode::RandomFromPool", EditConditionHides))
	TObjectPtr<UStageRewardPoolDefinition> RewardPoolDefinition = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_WorldSpawnMesh)
	TObjectPtr<UStaticMesh> WorldSpawnMeshOverride = nullptr;

	UPROPERTY()
	bool bInteractionConsumed = false;

private:
	UFUNCTION()
	void OnRep_WorldSpawnMesh();

	void AssignRandomRewardDefinition();
	URewardDefinition* PickWeightedRewardDefinition() const;
};
