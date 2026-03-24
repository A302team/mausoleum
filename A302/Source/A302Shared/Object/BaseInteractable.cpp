#include "Object/BaseInteractable.h"

#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
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
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetGenerateOverlapEvents(true);

	// Fallback mesh so default ABaseInteractable spawn is visible even without a custom BP class.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMeshObj(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultMeshObj.Succeeded())
	{
		Mesh->SetStaticMesh(DefaultMeshObj.Object);
		Mesh->SetWorldScale3D(FVector(0.35f));
	}
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

void ABaseInteractable::SetRewardDefinition(URewardDefinition* InRewardDefinition)
{
	RewardAssignmentMode = ERewardAssignmentMode::Manual;
	RewardPoolDefinition = nullptr;
	RewardDefinition = InRewardDefinition;
	ForceNetUpdate();
}

void ABaseInteractable::SetInteractType(EInteractType InInteractType)
{
	CurrentInteractType = InInteractType;
	ForceNetUpdate();
}

void ABaseInteractable::SetWorldSpawnMesh(UStaticMesh* InWorldSpawnMesh)
{
	WorldSpawnMeshOverride = InWorldSpawnMesh;
	OnRep_WorldSpawnMesh();
	ForceNetUpdate();
}

void ABaseInteractable::OnRep_WorldSpawnMesh()
{
	if (Mesh && WorldSpawnMeshOverride)
	{
		Mesh->SetStaticMesh(WorldSpawnMeshOverride);
	}
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

void ABaseInteractable::RestoreInteraction()
{
	if (!HasAuthority() || !bInteractionConsumed)
	{
		return;
	}

	bInteractionConsumed = false;
	SetActorEnableCollision(true);
	SetActorHiddenInGame(false);
	ForceNetUpdate();
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

	FString RewardLabel = TEXT("None");
	if (RewardDefinition)
	{
		RewardLabel = !RewardDefinition->DisplayName.IsEmpty()
			? RewardDefinition->DisplayName.ToString()
			: RewardDefinition->ItemId.ToString();
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[Interaction] Success. reward=%s rewardAsset=%s actor=%s player=%s"),
		*RewardLabel,
		*GetNameSafe(RewardDefinition),
		*GetName(),
		*GetNameSafe(PlayerCharacter)
	);
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
	if (RewardDefinition)
	{
		if (!RewardDefinition->DisplayName.IsEmpty())
		{
			return FString::Printf(TEXT("%s (Interact)"), *RewardDefinition->DisplayName.ToString());
		}

		if (!RewardDefinition->ItemId.IsNone())
		{
			return FString::Printf(TEXT("%s (Interact)"), *RewardDefinition->ItemId.ToString());
		}

		return FString::Printf(TEXT("%s (Interact)"), *GetNameSafe(RewardDefinition));
	}

	// Reward replication is not ready yet (or not assigned): avoid exposing actor instance names like BaseInteractable_1.
	return TEXT("Reward Pending (Interact)");
}

EInteractType ABaseInteractable::GetInteractType()
{
	return CurrentInteractType;
}

void ABaseInteractable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseInteractable, CurrentInteractType);
	DOREPLIFETIME(ABaseInteractable, RewardDefinition);
	DOREPLIFETIME(ABaseInteractable, WorldSpawnMeshOverride);
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
