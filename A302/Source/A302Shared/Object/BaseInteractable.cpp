#include "Object/BaseInteractable.h"

#include "Components/StaticMeshComponent.h"
#include "GameData/Items/ItemDefinition.h"
#include "GameData/RewardDefinition.h"
#include "GameData/StageRewardPoolDefinition.h"
#include "GameFramework/Character.h"
#include "Math/UnrealMathUtility.h"
#include "Net/UnrealNetwork.h"

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

void ABaseInteractable::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		AssignRandomRewardDefinition();
	}
}

UItemDefinition* ABaseInteractable::GetItemDefinition() const
{
	return Cast<UItemDefinition>(RewardDefinition);
}

void ABaseInteractable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseInteractable, RewardDefinition);
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

void ABaseInteractable::ApplyStageRewardPoolDefinition(UStageRewardPoolDefinition* StageRewardPoolDefinition)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!StageRewardPoolDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Interaction] Stage reward pool definition is null. actor=%s"), *GetName());
		return;
	}

	RewardAssignmentMode = ERewardAssignmentMode::RandomFromPool;
	RewardPoolDefinition = StageRewardPoolDefinition;
	AssignRandomRewardDefinition();
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

void ABaseInteractable::AssignRandomRewardDefinition()
{
	if (RewardAssignmentMode != ERewardAssignmentMode::RandomFromPool)
	{
		return;
	}

	if (URewardDefinition* PickedRewardDefinition = PickWeightedRewardDefinition())
	{
		RewardDefinition = PickedRewardDefinition;
		ForceNetUpdate();
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Interaction] Random reward pool definition is empty or invalid. actor=%s pool=%s"), *GetName(), *GetNameSafe(RewardPoolDefinition));
}

URewardDefinition* ABaseInteractable::PickWeightedRewardDefinition() const
{
	if (!RewardPoolDefinition)
	{
		return nullptr;
	}

	const TArray<FWeightedRewardDefinitionEntry>& RewardPool = RewardPoolDefinition->RewardPool;
	float TotalWeight = 0.0f;
	for (const FWeightedRewardDefinitionEntry& Entry : RewardPool)
	{
		if (Entry.RewardDefinition && Entry.Weight > 0.0f)
		{
			TotalWeight += Entry.Weight;
		}
	}

	if (TotalWeight <= 0.0f)
	{
		return nullptr;
	}

	const float Roll = FMath::FRandRange(0.0f, TotalWeight);
	float AccumulatedWeight = 0.0f;

	for (const FWeightedRewardDefinitionEntry& Entry : RewardPool)
	{
		if (!Entry.RewardDefinition || Entry.Weight <= 0.0f)
		{
			continue;
		}

		AccumulatedWeight += Entry.Weight;
		if (Roll <= AccumulatedWeight)
		{
			return Entry.RewardDefinition;
		}
	}

	return RewardPool.Last().RewardDefinition;
}

