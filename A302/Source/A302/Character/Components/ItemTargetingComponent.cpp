#include "Character/Components/ItemTargetingComponent.h"

#include "Character/Components/ItemManagerComponent.h"
#include "Character/Components/QuickSlotComponent.h"
#include "Character/MyCharacter.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameData/ItemDefinition.h"
#include "GameFramework/Character.h"
#include "GamePlay/Items/BaseItem.h"
#include "Interface/UsableItem.h"

UItemTargetingComponent::UItemTargetingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UItemTargetingComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* OwnerActor = GetOwner())
	{
		ItemManagerComponent = OwnerActor->FindComponentByClass<UItemManagerComponent>();
		QuickSlotComponent = OwnerActor->FindComponentByClass<UQuickSlotComponent>();
	}
}

void UItemTargetingComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bEnableAttackRangeDebug)
	{
		UpdateAttackRangeDebugState();
	}
}

AActor* UItemTargetingComponent::FindTargetActorForUse(FVector& OutTargetLocation) const
{
	OutTargetLocation = FVector::ZeroVector;

	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		return nullptr;
	}

	const FVector Start = OwnerCharacter->GetPawnViewLocation();
	const FVector ForwardVector = OwnerCharacter->GetViewRotation().Vector();
	const FVector End = Start + (ForwardVector * AttackTraceDistance);

	OutTargetLocation = End;

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerCharacter);

	if (!GetWorld())
	{
		return nullptr;
	}

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
	{
		OutTargetLocation = HitResult.ImpactPoint;

		AActor* HitActor = HitResult.GetActor();
		if (HitActor && HitActor != OwnerCharacter && HitActor->IsA<ACharacter>())
		{
			return HitActor;
		}
	}

	TArray<FOverlapResult> OverlapResults;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	const bool bHasOverlap = GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		OwnerCharacter->GetActorLocation(),
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(AttackTraceDistance),
		Params
	);

	if (!bHasOverlap)
	{
		return nullptr;
	}

	AActor* BestTarget = nullptr;
	float BestScore = -FLT_MAX;

	const FVector OwnerForward = OwnerCharacter->GetViewRotation().Vector();
	for (const FOverlapResult& Overlap : OverlapResults)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!Candidate || Candidate == OwnerCharacter)
		{
			continue;
		}

		if (!Candidate->IsA<ACharacter>())
		{
			continue;
		}

		const FVector ToCandidate = Candidate->GetActorLocation() - OwnerCharacter->GetActorLocation();
		const float Distance = ToCandidate.Size();
		if (Distance <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		const float DotForward = FVector::DotProduct(OwnerForward, ToCandidate / Distance);
		if (DotForward < 0.1f)
		{
			continue;
		}

		const float Score = DotForward * 1000.0f - Distance;
		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}

	if (BestTarget)
	{
		OutTargetLocation = BestTarget->GetActorLocation();
	}

	return BestTarget;
}

bool UItemTargetingComponent::TryBuildTargetDataForUse(
	const UItemDefinition* ItemDefinition,
	FItemTargetData& OutTargetData,
	bool bForceTargetActor
) const
{
	OutTargetData.TargetActor = nullptr;
	OutTargetData.TargetLocation = FVector::ZeroVector;

	if (!ItemDefinition)
	{
		return false;
	}

	const bool bNeedsTargetActor = bForceTargetActor || ItemDefinition->Payload.UseMode == EItemUseMode::Targeted;
	if (!bNeedsTargetActor)
	{
		return true;
	}

	FVector TargetLocation = FVector::ZeroVector;
	AActor* TargetActor = FindTargetActorForUse(TargetLocation);
	if (!TargetActor)
	{
		return false;
	}

	OutTargetData.TargetActor = TargetActor;
	OutTargetData.TargetLocation = TargetLocation;
	return true;
}

void UItemTargetingComponent::UpdateAttackRangeDebugState()
{
	bool bIsTargetInRangeNow = false;
	FString TargetName;

	UItemManagerComponent* ItemManager = GetItemManager();
	UQuickSlotComponent* QuickSlot = GetQuickSlot();
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (ItemManager && QuickSlot && OwnerCharacter)
	{
		const int32 SelectedSlotIndex = QuickSlot->GetSelectedSlotIndex();
		if (ItemManager->IsValidSlotIndex(SelectedSlotIndex))
		{
			UItemDefinition* ItemDefinition = ItemManager->GetItemDefinitionAtSlot(SelectedSlotIndex);
			UBaseItem* ItemLogic = ItemManager->GetItemLogicAtSlot(SelectedSlotIndex);
			if (
				ItemDefinition &&
				ItemLogic &&
				ItemDefinition->Payload.UseMode == EItemUseMode::Targeted &&
				ItemLogic->GetClass()->ImplementsInterface(UUsableItem::StaticClass())
			)
			{
				FVector TargetLocation = FVector::ZeroVector;
				AActor* TargetActor = FindTargetActorForUse(TargetLocation);
				if (TargetActor)
				{
					FItemTargetData TargetData;
					TargetData.TargetActor = TargetActor;
					TargetData.TargetLocation = TargetLocation;

					bIsTargetInRangeNow = IUsableItem::Execute_CanUse(ItemLogic, OwnerCharacter, TargetData);
					TargetName = TargetActor->GetName();
				}
			}
		}
	}

	if (bIsTargetInRangeNow && !bWasAttackTargetInRange)
	{
		LogAndScreenMessage(
			FString::Printf(TEXT("[Targeting] Attack target in range: %s"), *TargetName),
			FColor::Green,
			1.2f
		);
	}

	bWasAttackTargetInRange = bIsTargetInRangeNow;
}

AMyCharacter* UItemTargetingComponent::GetOwnerCharacter() const
{
	return Cast<AMyCharacter>(GetOwner());
}

UItemManagerComponent* UItemTargetingComponent::GetItemManager() const
{
	UItemTargetingComponent* MutableThis = const_cast<UItemTargetingComponent*>(this);
	if (!MutableThis->ItemManagerComponent && MutableThis->GetOwner())
	{
		MutableThis->ItemManagerComponent = MutableThis->GetOwner()->FindComponentByClass<UItemManagerComponent>();
	}

	return MutableThis->ItemManagerComponent;
}

UQuickSlotComponent* UItemTargetingComponent::GetQuickSlot() const
{
	UItemTargetingComponent* MutableThis = const_cast<UItemTargetingComponent*>(this);
	if (!MutableThis->QuickSlotComponent && MutableThis->GetOwner())
	{
		MutableThis->QuickSlotComponent = MutableThis->GetOwner()->FindComponentByClass<UQuickSlotComponent>();
	}

	return MutableThis->QuickSlotComponent;
}

void UItemTargetingComponent::LogAndScreenMessage(
	const FString& Message,
	const FColor& Color,
	float Duration
) const
{
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, Duration, Color, Message);
	}
}
