#include "Object/BaseInteractable.h"

#include "Components/StaticMeshComponent.h"
#include "GameData/Items/ItemDefinition.h"
#include "GameData/RewardDefinition.h"
#include "GameFramework/Character.h"
#include "Math/UnrealMathUtility.h"

ABaseInteractable::ABaseInteractable()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	CurrentInteractType = EInteractType::Hold;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	RootComponent = Mesh;

	const int32 RandomIndex = FMath::RandRange(0, static_cast<int32>(EInteractType::MAX) - 1);
	CurrentInteractType = static_cast<EInteractType>(RandomIndex);
}

UItemDefinition* ABaseInteractable::GetItemDefinition() const
{
	return Cast<UItemDefinition>(RewardDefinition);
}

bool ABaseInteractable::TryConsumeInteraction()
{
	if (!HasAuthority() || bInteractionConsumed)
	{
		return false;
	}

	bInteractionConsumed = true;
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	ForceNetUpdate();
	return true;
}

void ABaseInteractable::Interact(ACharacter* PlayerCharacter)
{
	if (PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Interaction] %s interacted with %s!"), *PlayerCharacter->GetName(), *GetName());
	}
}

void ABaseInteractable::OnInteractionSuccess(ACharacter* PlayerCharacter)
{
	if (!PlayerCharacter)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Interaction] Success. actor=%s player=%s reward=%s"), *GetName(), *GetNameSafe(PlayerCharacter), *GetNameSafe(RewardDefinition));
}

FString ABaseInteractable::GetInteractText()
{
	if (RewardDefinition && !RewardDefinition->DisplayName.IsEmpty())
	{
		return FString::Printf(TEXT("%s (Interact)"), *RewardDefinition->DisplayName.ToString());
	}

	return FString::Printf(TEXT("%s (Interact)"), *GetName());
}

EInteractType ABaseInteractable::GetInteractType()
{
	return CurrentInteractType;
}


