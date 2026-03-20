#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CharacterRewardComponent.generated.h"

class AMyCharacter;
class AActor;
class URewardDefinition;
class UItemDefinition;
class ABaseInteractable;
enum class ERewardCategory : uint8;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302SHARED_API UCharacterRewardComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCharacterRewardComponent();

	bool HandleRewardPickup(AActor* InteractedActor, const URewardDefinition* RewardDefinition);
	void ResolveInteractionRewardOnServer(ABaseInteractable* Interactable);

	UFUNCTION(Server, Reliable)
	void Server_RequestInteractionReward(ABaseInteractable* Interactable);

	UFUNCTION(Server, Reliable)
	void Server_RequestTargetedItemUse(UItemDefinition* ItemDefinition, AActor* TargetActor);

	UFUNCTION(Client, Reliable)
	void Client_GrantInteractionReward(URewardDefinition* RewardDefinition);

protected:
	virtual void BeginPlay() override;

private:
	AMyCharacter* GetOwnerCharacter() const;

	ERewardCategory ResolveEffectiveRewardCategory(const URewardDefinition* RewardDefinition) const;

	bool ShouldGrantRewardLocally(const URewardDefinition* RewardDefinition) const;
	bool HandleBasicItemPickup(AActor* InteractedActor, const UItemDefinition* RewardDefinition);
	bool HandlePersonalEventPickup(AActor* InteractedActor, const URewardDefinition* RewardDefinition);
	bool HandleGroupEventPickup(AActor* InteractedActor, const URewardDefinition* RewardDefinition);
};
