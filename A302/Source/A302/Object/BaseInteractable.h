#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/InteractableInterface.h"
#include "BaseInteractable.generated.h"

class UItemDefinition;
class UBaseEvent;
class AMyCharacter;

UENUM(BlueprintType)
enum class ERewardType : uint8
{
	None        UMETA(DisplayName = "None"),
	Item        UMETA(DisplayName = "Item"),
	Event       UMETA(DisplayName = "Event")
};

UCLASS()
class A302_API ABaseInteractable : public AActor, public IInteractableInterface
{
	GENERATED_BODY()
		
public:	
	ABaseInteractable();

	UItemDefinition* GetItemDefinition() const { return RewardItem; }
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	virtual void OnInteractionSuccess(AMyCharacter* PlayerCharacter);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	EInteractType CurrentInteractType;

	virtual void Interact(AMyCharacter* PlayerCharacter) override;
	virtual FString GetInteractText() override;
	virtual EInteractType GetInteractType() override;
	
	// 메시
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> Mesh;
    
	// 보상
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reward")
	ERewardType RewardType = ERewardType::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward", meta = (EditCondition = "RewardType == ERewardType::Item", EditConditionHides))
	TObjectPtr<UItemDefinition> RewardItem;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward", meta = (EditCondition = "RewardType == ERewardType::Event", EditConditionHides))
	TSubclassOf<UBaseEvent> RewardEventClass;
};
