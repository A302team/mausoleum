#include "GamePlay/Events/PersonalEvents/PersonalEventTimeKnife.h"

#include "Character/Components/ItemManagerComponent.h"
#include "Character/Components/QuickSlotComponent.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "GameData/ItemDefinition.h"
#include "GamePlay/Items/ItemTimeKnife.h"
#include "Engine/World.h"

void UPersonalEventTimeKnife::ExecuteEvent_Implementation(AMyCharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter)
	{
		return;
	}

	StopCountdown(false);

	OwnerCharacter = InstigatorCharacter;

	const UItemDefinition* ItemDef = GetRewardDefinition();
	RemainingSeconds = ItemDef ? FMath::Max(1.0f, ItemDef->TimedKillDuration) : 30.0f;

	UItemDefinition* GrantedKnifeDefinition = ResolveGrantedKnifeDefinition(ItemDef);
	if (!GrantedKnifeDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PersonalEventTimeKnife] Granted knife definition is missing."));
		OwnerCharacter = nullptr;
		return;
	}

	UItemManagerComponent* ItemManagerComponent = InstigatorCharacter->FindComponentByClass<UItemManagerComponent>();
	int32 AddedSlotIndex = INDEX_NONE;
	if (!ItemManagerComponent || !ItemManagerComponent->TryAddItemToFirstEmptySlot(GrantedKnifeDefinition, 1, AddedSlotIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PersonalEventTimeKnife] Failed to grant timed knife: add failed or slot full."));
		OwnerCharacter = nullptr;
		return;
	}

	if (UQuickSlotComponent* QuickSlotComponent = InstigatorCharacter->FindComponentByClass<UQuickSlotComponent>())
	{
		QuickSlotComponent->NotifyItemAddedToSlot(AddedSlotIndex, GrantedKnifeDefinition);
	}

	GrantedItemId = GrantedKnifeDefinition->ItemId;
	bIsActive = true;

	InstigatorCharacter->RegisterActiveTimedKnifeEvent(this);
	RefreshTimerUI();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CountdownTimerHandle);
		World->GetTimerManager().SetTimer(
			CountdownTimerHandle,
			this,
			&UPersonalEventTimeKnife::HandleCountdownTick,
			1.0f,
			true
		);
	}

	UE_LOG(LogTemp, Log, TEXT("[PersonalEventTimeKnife] Countdown started: %.0fs"), RemainingSeconds);
}

void UPersonalEventTimeKnife::NotifyKillConfirmed()
{
	if (!bIsActive)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[PersonalEventTimeKnife] Objective success."));
	StopCountdown(true);
}

void UPersonalEventTimeKnife::CancelCountdown()
{
	if (!bIsActive)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[PersonalEventTimeKnife] Countdown canceled."));
	StopCountdown(true);
}

void UPersonalEventTimeKnife::BeginDestroy()
{
	StopCountdown(true);
	Super::BeginDestroy();
}

void UPersonalEventTimeKnife::HandleCountdownTick()
{
	if (!bIsActive)
	{
		return;
	}

	AMyCharacter* Character = OwnerCharacter.Get();
	if (!Character)
	{
		StopCountdown(true);
		return;
	}

	RemainingSeconds = FMath::Max(0.0f, RemainingSeconds - 1.0f);
	RefreshTimerUI();

	if (RemainingSeconds > 0.0f)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[PersonalEventTimeKnife] Time expired."));
	StopCountdown(true);
	Character->ForceDeadByPersonalEvent();
}

void UPersonalEventTimeKnife::RefreshTimerUI() const
{
	AMyCharacter* Character = OwnerCharacter.Get();
	if (!Character)
	{
		return;
	}

	if (AMyPlayerController* PlayerController = Cast<AMyPlayerController>(Character->GetController()))
	{
		PlayerController->UpdateItemTimerText(RemainingSeconds);
		PlayerController->SetItemTimerVisible(true);
	}
}

void UPersonalEventTimeKnife::StopCountdown(bool bHideTimer)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CountdownTimerHandle);
	}

	AMyCharacter* Character = OwnerCharacter.Get();
	if (Character)
	{
		Character->ClearActiveTimedKnifeEvent(this);

		if (!GrantedItemId.IsNone())
		{
			int32 RemovedSlotIndex = INDEX_NONE;
			if (UItemManagerComponent* ItemManagerComponent = Character->FindComponentByClass<UItemManagerComponent>())
			{
				if (ItemManagerComponent->RemoveFirstItemByItemId(GrantedItemId, RemovedSlotIndex))
				{
					if (RemovedSlotIndex != INDEX_NONE)
					{
						if (UQuickSlotComponent* QuickSlotComponent = Character->FindComponentByClass<UQuickSlotComponent>())
						{
							QuickSlotComponent->NotifyItemRemovedFromSlot(RemovedSlotIndex);
						}
					}
				}
			}
		}

		if (bHideTimer)
		{
			if (AMyPlayerController* PlayerController = Cast<AMyPlayerController>(Character->GetController()))
			{
				PlayerController->SetItemTimerVisible(false);
			}
		}
	}

	OwnerCharacter = nullptr;
	GrantedItemId = NAME_None;
	RemainingSeconds = 0.0f;
	bIsActive = false;
}

UItemDefinition* UPersonalEventTimeKnife::ResolveGrantedKnifeDefinition(const UItemDefinition* EventDefinition) const
{
	if (!EventDefinition)
	{
		return nullptr;
	}

	UItemDefinition* GrantedDefinition = EventDefinition->GrantedItemDefinition.Get();
	if (GrantedDefinition)
	{
		UClass* GrantedLogicClass = GrantedDefinition->ResolveRewardLogicClass();
		const bool bIsValidTimedKnifeItem =
			GrantedDefinition->RewardCategory == ERewardCategory::BasicItem &&
			GrantedLogicClass &&
			GrantedLogicClass->IsChildOf(UItemTimeKnife::StaticClass());

		if (bIsValidTimedKnifeItem)
		{
			return GrantedDefinition;
		}
	}

	UClass* EventLogicClass = EventDefinition->ResolveRewardLogicClass();
	const bool bLegacySelfTimedKnife =
		EventDefinition->RewardCategory == ERewardCategory::BasicItem &&
		EventLogicClass &&
		EventLogicClass->IsChildOf(UItemTimeKnife::StaticClass());

	if (bLegacySelfTimedKnife)
	{
		return const_cast<UItemDefinition*>(EventDefinition);
	}

	return nullptr;
}
