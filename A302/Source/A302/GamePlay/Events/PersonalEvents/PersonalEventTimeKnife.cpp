#include "GamePlay/Events/PersonalEvents/PersonalEventTimeKnife.h"

#include "Character/Components/ItemManagerComponent.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "GameData/Items/ItemDefinition.h"
#include "GameData/Events/PersonalEvents/PersonalEventTimeKnifeDefinition.h"
#include "GameData/RewardDefinition.h"
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

	const URewardDefinition* SourceRewardDefinition = GetRewardDefinition();
	const UPersonalEventTimeKnifeDefinition* EventDef =
		Cast<UPersonalEventTimeKnifeDefinition>(const_cast<URewardDefinition*>(SourceRewardDefinition));
	RemainingSeconds = EventDef ? FMath::Max(1.0f, EventDef->Payload.TimedKillDuration) : 30.0f;

	UItemDefinition* GrantedKnifeDefinition = ResolveGrantedKnifeDefinition(SourceRewardDefinition, EventDef);
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
				ItemManagerComponent->RemoveFirstItemByItemId(GrantedItemId, RemovedSlotIndex);
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

UItemDefinition* UPersonalEventTimeKnife::ResolveGrantedKnifeDefinition(const URewardDefinition* SourceRewardDefinition, const UPersonalEventTimeKnifeDefinition* EventDefinition) const
{
	auto IsValidTimedKnifeItem = [](const UItemDefinition* Candidate)
	{
		if (!Candidate)
		{
			return false;
		}

		UClass* CandidateLogicClass = Candidate->ResolveRewardLogicClass();
		return CandidateLogicClass && CandidateLogicClass->IsChildOf(UItemTimeKnife::StaticClass());
	};

	// Primary path: explicit event payload points to the granted timed-knife item.
	if (EventDefinition)
	{
		UItemDefinition* GrantedDefinition = EventDefinition->Payload.GrantedItemDefinition.Get();
		if (IsValidTimedKnifeItem(GrantedDefinition))
		{
			return GrantedDefinition;
		}
	}

	// Legacy/single-asset path: allow the picked reward itself to be a timed-knife item definition.
	UItemDefinition* SourceAsItemDefinition = Cast<UItemDefinition>(const_cast<URewardDefinition*>(SourceRewardDefinition));
	if (IsValidTimedKnifeItem(SourceAsItemDefinition))
	{
		return SourceAsItemDefinition;
	}

	return nullptr;
}


