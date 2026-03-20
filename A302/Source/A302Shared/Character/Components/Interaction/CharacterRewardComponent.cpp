#include "Character/Components/Interaction/CharacterRewardComponent.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "Character/Components/Inventory/ItemManagerComponent.h"
#include "Character/Components/Combat/CombatStatusComponent.h"
#include "GameData/RewardDefinition.h"
#include "GameData/Items/ItemDefinition.h"
#include "GameData/Events/PersonalEvents/Malice/PersonalEventInspectMaliceDefinition.h"
#include "GameData/Events/PersonalEvents/Equipment/PersonalEventCursedSwordDefinition.h"
#include "GamePlay/Items/BaseItem.h"
#include "GamePlay/Items/ItemShield.h"
#include "GamePlay/Items/ItemCursedSword.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"
#include "GamePlay/Events/GroupEvents/BaseGroupEvent.h"
#include "Object/BaseInteractable.h"
#include "Interface/A302ServerRewardBridge.h"
#include "A302GameplayGuards.h"
#include "GameFramework/GameModeBase.h"

namespace
{
	bool TryExecutePersonalEventWithoutBridge(ACharacter* InstigatorCharacter, AActor* InteractedActor, const URewardDefinition* RewardDefinition)
	{
		if (!InstigatorCharacter || !RewardDefinition)
		{
			return false;
		}

		UClass* LogicClass = RewardDefinition->ResolveRewardLogicClass();
		if (!LogicClass || !LogicClass->IsChildOf(UBasePersonalEvent::StaticClass()))
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("[Reward] Direct personal event fallback failed: invalid logic. reward=%s logic=%s"),
				*GetNameSafe(RewardDefinition),
				*GetNameSafe(LogicClass)
			);
			return false;
		}

		UBasePersonalEvent* PersonalEvent = NewObject<UBasePersonalEvent>(InstigatorCharacter, LogicClass);
		if (!PersonalEvent)
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("[Reward] Direct personal event fallback failed: NewObject returned null. reward=%s logic=%s"),
				*GetNameSafe(RewardDefinition),
				*GetNameSafe(LogicClass)
			);
			return false;
		}

		PersonalEvent->EventID = RewardDefinition->ItemId;
		PersonalEvent->InitializeRuntimeContext(InstigatorCharacter);
		PersonalEvent->InitializeContext(RewardDefinition, InteractedActor);
		PersonalEvent->ExecuteEvent(InstigatorCharacter);
		return true;
	}

	bool TryExecuteGroupEventWithoutBridge(ACharacter* InstigatorCharacter, AActor* InteractedActor, const URewardDefinition* RewardDefinition)
	{
		if (!InstigatorCharacter || !InstigatorCharacter->HasAuthority() || !RewardDefinition)
		{
			return false;
		}

		UClass* LogicClass = RewardDefinition->ResolveRewardLogicClass();
		if (!LogicClass || !LogicClass->IsChildOf(UBaseGroupEvent::StaticClass()))
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("[Reward] Direct group event fallback failed: invalid logic. reward=%s logic=%s"),
				*GetNameSafe(RewardDefinition),
				*GetNameSafe(LogicClass)
			);
			return false;
		}

		UObject* GroupEventOuter = InstigatorCharacter->GetWorld()
			? InstigatorCharacter->GetWorld()->GetAuthGameMode()
			: nullptr;
		if (!GroupEventOuter)
		{
			GroupEventOuter = InstigatorCharacter;
		}

		UBaseGroupEvent* GroupEvent = NewObject<UBaseGroupEvent>(GroupEventOuter, LogicClass);
		if (!GroupEvent)
		{
			UE_LOG(
				LogTemp,
				Error,
				TEXT("[Reward] Direct group event fallback failed: NewObject returned null. reward=%s logic=%s"),
				*GetNameSafe(RewardDefinition),
				*GetNameSafe(LogicClass)
			);
			return false;
		}

		GroupEvent->EventID = RewardDefinition->ItemId;
		GroupEvent->InitializeRuntimeContext(InstigatorCharacter);
		GroupEvent->InitializeContext(RewardDefinition, InteractedActor);
		GroupEvent->ExecuteEvent(InstigatorCharacter);
		return true;
	}
}

UCharacterRewardComponent::UCharacterRewardComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UCharacterRewardComponent::BeginPlay()
{
	Super::BeginPlay();
}

AMyCharacter* UCharacterRewardComponent::GetOwnerCharacter() const
{
	return Cast<AMyCharacter>(GetOwner());
}

bool UCharacterRewardComponent::HandleRewardPickup(AActor* InteractedActor, const URewardDefinition* RewardDefinition)
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter) return false;

	if (!A302GameplayGuards::IsGameplayEnabledCharacter(OwnerCharacter))
	{
		return false;
	}

	if (!RewardDefinition)
	{
		return false;
	}

	ERewardCategory EffectiveCategory = RewardDefinition->RewardCategory;
	UClass* LogicClass = RewardDefinition->ResolveRewardLogicClass();
	if (LogicClass)
	{
		const bool bIsLegacyPersonalEventClass =
			RewardDefinition->RewardCategory != ERewardCategory::BasicItem &&
			LogicClass->IsChildOf(UItemCursedSword::StaticClass());

		if (LogicClass->IsChildOf(UBasePersonalEvent::StaticClass()) || bIsLegacyPersonalEventClass)
		{
			EffectiveCategory = ERewardCategory::PersonalEvent;
		}
		else if (LogicClass->IsChildOf(UBaseGroupEvent::StaticClass()))
		{
			EffectiveCategory = ERewardCategory::GroupEvent;
		}
	}

	switch (EffectiveCategory)
	{
	case ERewardCategory::BasicItem:
	{
		const UItemDefinition* ItemDefinition = Cast<UItemDefinition>(const_cast<URewardDefinition*>(RewardDefinition));
		if (!ItemDefinition)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Reward] Basic item pickup failed: reward is not UItemDefinition. item=%s"), *GetNameSafe(RewardDefinition));
			return false;
		}
		return HandleBasicItemPickup(InteractedActor, ItemDefinition);
	}

	case ERewardCategory::PersonalEvent:
		return HandlePersonalEventPickup(InteractedActor, RewardDefinition);

	case ERewardCategory::GroupEvent:
		return HandleGroupEventPickup(InteractedActor, RewardDefinition);

	default:
		UE_LOG(LogTemp, Warning, TEXT("[Reward] Unknown category. item=%s"), *GetNameSafe(RewardDefinition));
		return false;
	}
}

bool UCharacterRewardComponent::ShouldGrantRewardLocally(const URewardDefinition* RewardDefinition) const
{
	if (!RewardDefinition)
	{
		return false;
	}

	if (RewardDefinition->RewardCategory == ERewardCategory::BasicItem)
	{
		return true;
	}

	UClass* LogicClass = RewardDefinition->ResolveRewardLogicClass();
	if (LogicClass && LogicClass->IsChildOf(UItemCursedSword::StaticClass()))
	{
		return true;
	}

	URewardDefinition* MutableRewardDefinition = const_cast<URewardDefinition*>(RewardDefinition);
	return
		Cast<UPersonalEventInspectMaliceDefinition>(MutableRewardDefinition) != nullptr;
}

void UCharacterRewardComponent::ResolveInteractionRewardOnServer(ABaseInteractable* Interactable)
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority() || !IsValid(Interactable))
	{
		return;
	}

	if (!Interactable->TryConsumeInteraction())
	{
		return;
	}

	const URewardDefinition* RewardDefinition = Interactable->GetRewardDefinition();
	if (RewardDefinition)
	{
		if (ShouldGrantRewardLocally(RewardDefinition))
		{
			Client_GrantInteractionReward(const_cast<URewardDefinition*>(RewardDefinition));
		}
		else
		{
			HandleRewardPickup(Interactable, RewardDefinition);
		}
	}

	Interactable->ForceNetUpdate();
	Interactable->Destroy();
}

void UCharacterRewardComponent::Server_RequestInteractionReward_Implementation(ABaseInteractable* Interactable)
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority() || !IsValid(Interactable))
	{
		return;
	}

	if (!A302GameplayGuards::IsGameplayEnabledCharacter(OwnerCharacter))
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Interaction] Server rejected reward: gameplay not enabled for player=%s"), *GetNameSafe(OwnerCharacter));
		return;
	}

	const float MaxAllowedDistance = FMath::Max(OwnerCharacter->InteractionDistance, 0.f) + 200.f;
	if (OwnerCharacter->GetDistanceTo(Interactable) > MaxAllowedDistance)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Interaction] Server rejected interactable reward: too far. player=%s actor=%s"),
			*GetNameSafe(OwnerCharacter),
			*GetNameSafe(Interactable)
		);
		return;
	}

	ResolveInteractionRewardOnServer(Interactable);
}

void UCharacterRewardComponent::Server_RequestTargetedItemUse_Implementation(UItemDefinition* ItemDefinition, AActor* TargetActor)
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority() || !ItemDefinition || !IsValid(TargetActor))
	{
		return;
	}

	UClass* LogicClass = ItemDefinition->ResolveRewardLogicClass();
	if (!LogicClass || !LogicClass->IsChildOf(UBaseItem::StaticClass()))
	{
		return;
	}

	const UBaseItem* ItemLogic = Cast<UBaseItem>(LogicClass->GetDefaultObject());
	if (!ItemLogic)
	{
		return;
	}

	FString SystemMessage;
	if (!ItemLogic->ResolveServerTargetedUse(OwnerCharacter, TargetActor, SystemMessage))
	{
		return;
	}

	if (!SystemMessage.IsEmpty())
	{
		if (AMyPlayerController* OwnerPlayerController = Cast<AMyPlayerController>(OwnerCharacter->GetController()))
		{
			OwnerPlayerController->Client_ReceiveSystemMessage(SystemMessage);
		}
	}
}

void UCharacterRewardComponent::Client_GrantInteractionReward_Implementation(URewardDefinition* RewardDefinition)
{
	HandleRewardPickup(nullptr, RewardDefinition);
}

bool UCharacterRewardComponent::HandleBasicItemPickup(AActor* InteractedActor, const UItemDefinition* RewardDefinition)
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter) return false;

	if (!RewardDefinition || RewardDefinition->RewardCategory != ERewardCategory::BasicItem)
	{
		return false;
	}

	UItemManagerComponent* ItemManagerComponent = OwnerCharacter->FindComponentByClass<UItemManagerComponent>();
	if (!ItemManagerComponent)
	{
		return false;
	}

	int32 AddedSlotIndex = INDEX_NONE;
	UItemDefinition* MutableDefinition = const_cast<UItemDefinition*>(RewardDefinition);
	if (ItemManagerComponent->TryAddItemToFirstEmptySlot(MutableDefinition, 1, AddedSlotIndex))
	{
		UClass* LogicClass = RewardDefinition->ResolveRewardLogicClass();
		
		if (LogicClass && LogicClass->IsChildOf(UBaseItem::StaticClass()))
		{
			if (const UBaseItem* BaseItemLogic = Cast<UBaseItem>(LogicClass->GetDefaultObject()))
			{
				BaseItemLogic->OnItemAcquired(OwnerCharacter);
			}
		}
		
		const bool bIsShieldItem = RewardDefinition && LogicClass && LogicClass->IsChildOf(UItemShield::StaticClass());
		if (bIsShieldItem)
		{
			if (UCombatStatusComponent* CombatStatusComponent = OwnerCharacter->FindComponentByClass<UCombatStatusComponent>())
			{
				CombatStatusComponent->AddShield(FMath::Max(1, RewardDefinition->Payload.BlockCount));
			}
		}

		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Reward] Basic item pickup failed: add failed or slot full. item=%s"), *GetNameSafe(RewardDefinition));
	return false;
}

bool UCharacterRewardComponent::HandlePersonalEventPickup(AActor* InteractedActor, const URewardDefinition* RewardDefinition)
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter) return false;

	if (!RewardDefinition)
	{
		return false;
	}

	UClass* LogicClass = RewardDefinition->ResolveRewardLogicClass();
	const bool bIsPersonalEventClass = LogicClass && LogicClass->IsChildOf(UBasePersonalEvent::StaticClass());
	const bool bIsLegacyCursedSwordLogic =
		RewardDefinition->RewardCategory != ERewardCategory::BasicItem &&
		LogicClass &&
		LogicClass->IsChildOf(UItemCursedSword::StaticClass());
	const bool bIsCursedSwordDefinition =
		Cast<UPersonalEventCursedSwordDefinition>(const_cast<URewardDefinition*>(RewardDefinition)) != nullptr;

	if (!bIsPersonalEventClass && !bIsLegacyCursedSwordLogic && !bIsCursedSwordDefinition)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[Event] Invalid personal event reward logic. item=%s class=%s"),
			*GetNameSafe(RewardDefinition),
			*GetNameSafe(LogicClass)
		);
		return false;
	}

	if (IA302ServerRewardBridge* RewardBridge = Cast<IA302ServerRewardBridge>(GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr))
	{
		return RewardBridge->TryHandlePersonalEventReward(OwnerCharacter, InteractedActor, RewardDefinition);
	}

	return TryExecutePersonalEventWithoutBridge(OwnerCharacter, InteractedActor, RewardDefinition);
}

bool UCharacterRewardComponent::HandleGroupEventPickup(AActor* InteractedActor, const URewardDefinition* RewardDefinition)
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter) return false;

	if (IA302ServerRewardBridge* RewardBridge = Cast<IA302ServerRewardBridge>(GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr))
	{
		return RewardBridge->TryHandleGroupEventReward(OwnerCharacter, InteractedActor, RewardDefinition);
	}

	return TryExecuteGroupEventWithoutBridge(OwnerCharacter, InteractedActor, RewardDefinition);
}
