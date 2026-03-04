#include "Character/Components/KnifeAutoTestComponent.h"

#include "Character/DummyCharacter.h"
#include "Character/MyCharacter.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameData/ItemDefinition.h"
#include "GameData/ItemInstance.h"
#include "GameData/ItemTypes.h"
#include "GamePlay/Items/BaseItem.h"
#include "Interface/UsableItem.h"
#include "Kismet/GameplayStatics.h"
#include "Manager/ItemActionFactory.h"
#include "TimerManager.h"

namespace
{
	void LogAndScreenKnife(const FString& Message, const FColor& Color = FColor::Yellow, const float Duration = 3.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, Duration, Color, Message);
		}
	}
}

UKnifeAutoTestComponent::UKnifeAutoTestComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UKnifeAutoTestComponent::StartAutoKnifeTest()
{
	if (!bEnableAutoKnifeTest)
	{
		return;
	}

	SetupKnifeForTest();
	FindAndWarpNearDummy();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (FirstAutoAttackDelay >= 0.f)
	{
		FTimerHandle TimerHandle;
		World->GetTimerManager().SetTimer(
			TimerHandle,
			this,
			&UKnifeAutoTestComponent::ExecuteAutoKnifeAttack,
			FirstAutoAttackDelay,
			false
		);
	}

	if (SecondAutoAttackDelay >= 0.f)
	{
		FTimerHandle TimerHandle;
		World->GetTimerManager().SetTimer(
			TimerHandle,
			this,
			&UKnifeAutoTestComponent::ExecuteAutoKnifeAttack,
			SecondAutoAttackDelay,
			false
		);
	}
}

AMyCharacter* UKnifeAutoTestComponent::GetOwnerCharacter() const
{
	return Cast<AMyCharacter>(GetOwner());
}

void UKnifeAutoTestComponent::SetupKnifeForTest()
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		return;
	}

	ItemActionFactory = NewObject<UItemActionFactory>(this);
	if (!ItemActionFactory)
	{
		UE_LOG(LogTemp, Error, TEXT("[KnifeAutoTest] Failed to create UItemActionFactory."));
		return;
	}

	if (!KnifeDef)
	{
		UE_LOG(LogTemp, Warning, TEXT("[KnifeAutoTest] KnifeDef is not set in editor."));
		return;
	}

	KnifeInstance = NewObject<UItemInstance>(this);
	if (!KnifeInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("[KnifeAutoTest] Failed to create UItemInstance."));
		return;
	}

	KnifeInstance->Init(KnifeDef, FMath::Max(0, InitialKnifeStack));
	LogAndScreenKnife(FString::Printf(TEXT("Knife Stack=%d set"), KnifeInstance->StackCount));

	KnifeLogic = ItemActionFactory->CreateLogic(this, KnifeInstance);
	if (!KnifeLogic)
	{
		UE_LOG(LogTemp, Error, TEXT("[KnifeAutoTest] Failed to create KnifeLogic from KnifeDef."));
	}
}

void UKnifeAutoTestComponent::FindAndWarpNearDummy()
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		return;
	}

	CachedDummyCharacter = Cast<ADummyCharacter>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ADummyCharacter::StaticClass())
	);

	if (!CachedDummyCharacter)
	{
		LogAndScreenKnife(TEXT("[KnifeAutoTest] DummyCharacter not found."), FColor::Orange);
		return;
	}

	const FVector DummyLocation = CachedDummyCharacter->GetActorLocation();
	const FVector WarpLocation = DummyLocation - CachedDummyCharacter->GetActorForwardVector() * AutoWarpDistanceToDummy;
	OwnerCharacter->SetActorLocation(WarpLocation);
}

void UKnifeAutoTestComponent::ExecuteAutoKnifeAttack()
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		return;
	}

	++AutoAttackCount;

	if (!KnifeLogic)
	{
		LogAndScreenKnife(
			FString::Printf(TEXT("[KnifeAutoTest] AutoAttack %d skipped: KnifeLogic is null."), AutoAttackCount),
			FColor::Orange
		);
		return;
	}

	if (!CachedDummyCharacter)
	{
		FindAndWarpNearDummy();
		if (!CachedDummyCharacter)
		{
			LogAndScreenKnife(
				FString::Printf(TEXT("[KnifeAutoTest] AutoAttack %d skipped: no dummy target."), AutoAttackCount),
				FColor::Orange
			);
			return;
		}
	}

	if (KnifeInstance && KnifeInstance->IsEmpty())
	{
		LogAndScreenKnife(
			FString::Printf(TEXT("[KnifeAutoTest] AutoAttack %d skipped: Knife stack is empty."), AutoAttackCount),
			FColor::Orange
		);
		return;
	}

	FItemTargetData TargetData;
	TargetData.TargetActor = CachedDummyCharacter;
	TargetData.TargetLocation = CachedDummyCharacter->GetActorLocation();

	const bool bUsed = IUsableItem::Execute_Use(KnifeLogic, OwnerCharacter, TargetData);
	if (!bUsed)
	{
		LogAndScreenKnife(
			FString::Printf(TEXT("[KnifeAutoTest] AutoAttack %d failed: Use() returned false."), AutoAttackCount),
			FColor::Orange
		);
	}
}
