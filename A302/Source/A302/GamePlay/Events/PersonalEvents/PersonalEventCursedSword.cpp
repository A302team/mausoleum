#include "GamePlay/Events/PersonalEvents/PersonalEventCursedSword.h"

#include "Character/Components/ItemManagerComponent.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "GameData/Items/ItemDefinition.h"
#include "GameData/Events/PersonalEvents/PersonalEventCursedSwordDefinition.h"
#include "GameData/Events/PersonalEvents/PersonalEventDefinition.h" 
#include "GameData/RewardDefinition.h"
#include "GamePlay/Items/BaseItem.h"
#include "GamePlay/Items/ItemTimeKnife.h"
#include "Engine/World.h"

namespace
{
	constexpr int32 VisibleQuickSlotCount = 5;

	FText BuildCursedSwordCountdownContext(float RemainingSeconds)
	{
		const int32 SafeSeconds = FMath::Max(0, FMath::CeilToInt(RemainingSeconds));
		return FText::FromString(FString::Printf(TEXT("%d초 안에 사용하지 않을 시 죽습니다."), SafeSeconds));
	}

	FText ResolveCursedSwordTitle(const UItemDefinition* GrantedKnifeDefinition, const UPersonalEventCursedSwordDefinition* EventDef)
	{
		if (GrantedKnifeDefinition && !GrantedKnifeDefinition->DisplayName.IsEmpty())
		{
			return GrantedKnifeDefinition->DisplayName;
		}

		if (EventDef && !EventDef->DisplayName.IsEmpty())
		{
			return EventDef->DisplayName;
		}

		return FText::FromString(TEXT("저주받은 검 획득"));
	}
}

void UPersonalEventCursedSword::ExecuteEvent_Implementation(AMyCharacter* InstigatorCharacter)
{
	BeginCursedSwordFlow(InstigatorCharacter);
}

void UPersonalEventCursedSword::OnEventResolved_Implementation(AMyCharacter* InstigatorCharacter, int32 ChoiceIndex)
{
	if (ChoiceIndex != 0)
	{
		return;
	}

	BeginCursedSwordFlow(InstigatorCharacter);
}

bool UPersonalEventCursedSword::BeginCursedSwordFlow(AMyCharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter || !InstigatorCharacter->HasAuthority() || bIsActive)
	{
		return false;
	}

	StopCountdown(false);
	OwnerCharacter = InstigatorCharacter;

	const URewardDefinition* SourceRewardDefinition = GetRewardDefinition();
	const UPersonalEventCursedSwordDefinition* EventDef =
	   Cast<UPersonalEventCursedSwordDefinition>(const_cast<URewardDefinition*>(SourceRewardDefinition));
	RemainingSeconds = EventDef ? FMath::Max(1.0f, EventDef->Payload.TimedKillDuration) : 30.0f;

	UItemDefinition* GrantedKnifeDefinition = ResolveGrantedKnifeDefinition(SourceRewardDefinition, EventDef);
	if (!GrantedKnifeDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PersonalEventTimeKnife] Granted knife definition is missing."));
		OwnerCharacter = nullptr;
		return false;
	}

	int32 AddedSlotIndex = INDEX_NONE;
	if (!TryGrantKnifeToPreferredSlot(InstigatorCharacter, GrantedKnifeDefinition, AddedSlotIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PersonalEventTimeKnife] Failed to grant timed knife: add failed or slot full."));
		OwnerCharacter = nullptr;
		return false;
	}

	UClass* LogicClass = GrantedKnifeDefinition->ResolveRewardLogicClass();
	if (LogicClass && LogicClass->IsChildOf(UBaseItem::StaticClass()))
	{
		if (const UBaseItem* BaseItemLogic = Cast<UBaseItem>(LogicClass->GetDefaultObject()))
		{
			BaseItemLogic->OnItemAcquired(InstigatorCharacter);
		}
	}

	GrantedItemId = GrantedKnifeDefinition->ItemId;
	GrantedSlotIndex = AddedSlotIndex;
	bIsActive = true;

	if (AMyPlayerController* PC = Cast<AMyPlayerController>(InstigatorCharacter->GetController()))
	{
		PC->Client_SetQuickSlotItemVisual(
			AddedSlotIndex,
			ResolveCursedSwordTitle(GrantedKnifeDefinition, EventDef),
			GrantedKnifeDefinition->Icon,
			true);

		PC->Client_ShowTitleCard(
			ResolveCursedSwordTitle(GrantedKnifeDefinition, EventDef),
			BuildCursedSwordCountdownContext(RemainingSeconds),
			0.0f);
	}

	InstigatorCharacter->RegisterActiveTimedKnifeEvent(this);
	RefreshTimerUI();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CountdownTimerHandle);
		World->GetTimerManager().SetTimer(
		   CountdownTimerHandle,
		   this,
		   &UPersonalEventCursedSword::HandleCountdownTick,
		   1.0f,
		   true
		);
	}

	UE_LOG(LogTemp, Log, TEXT("[PersonalEventTimeKnife] Countdown started: %.0fs"), RemainingSeconds);
	return true;
}

void UPersonalEventCursedSword::NotifyKillConfirmed()
{
	if (!bIsActive)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[PersonalEventTimeKnife] Objective success."));
	StopCountdown(true);
}

void UPersonalEventCursedSword::CancelCountdown()
{
	if (!bIsActive)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[PersonalEventTimeKnife] Countdown canceled."));
	StopCountdown(true);
}

void UPersonalEventCursedSword::BeginDestroy()
{
	StopCountdown(true);
	Super::BeginDestroy();
}

void UPersonalEventCursedSword::HandleCountdownTick()
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

void UPersonalEventCursedSword::RefreshTimerUI() const
{
	AMyCharacter* Character = OwnerCharacter.Get();
	if (!Character)
	{
		return;
	}

	if (AMyPlayerController* PlayerController = Cast<AMyPlayerController>(Character->GetController()))
	{
		const URewardDefinition* SourceRewardDefinition = GetRewardDefinition();
		const UPersonalEventCursedSwordDefinition* EventDefinition = Cast<UPersonalEventCursedSwordDefinition>(const_cast<URewardDefinition*>(SourceRewardDefinition));
		const UItemDefinition* GrantedKnifeDefinition = ResolveGrantedKnifeDefinition(SourceRewardDefinition, EventDefinition);

		PlayerController->Client_ShowTitleCard(
			ResolveCursedSwordTitle(GrantedKnifeDefinition, EventDefinition),
			BuildCursedSwordCountdownContext(RemainingSeconds),
			0.0f);
	}
}

void UPersonalEventCursedSword::StopCountdown(bool bHideTimer)
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

		if (GrantedSlotIndex != INDEX_NONE)
		{
			if (AMyPlayerController* PlayerController = Cast<AMyPlayerController>(Character->GetController()))
			{
				PlayerController->Client_SetQuickSlotItemVisual(GrantedSlotIndex, FText::GetEmpty(), nullptr, false);
			}
		}

		if (bHideTimer)
		{
			if (AMyPlayerController* PlayerController = Cast<AMyPlayerController>(Character->GetController()))
			{
				PlayerController->Client_HideTitleCard();
			}
		}
	}

	OwnerCharacter = nullptr;
	GrantedItemId = NAME_None;
	GrantedSlotIndex = INDEX_NONE;
	RemainingSeconds = 0.0f;
	bIsActive = false;
}

bool UPersonalEventCursedSword::TryGrantKnifeToPreferredSlot(AMyCharacter* InstigatorCharacter, UItemDefinition* GrantedKnifeDefinition, int32& OutAddedSlotIndex) const
{
	OutAddedSlotIndex = INDEX_NONE;
	if (!InstigatorCharacter || !GrantedKnifeDefinition)
	{
		return false;
	}

	UItemManagerComponent* ItemManagerComponent = InstigatorCharacter->FindComponentByClass<UItemManagerComponent>();
	if (!ItemManagerComponent)
	{
		return false;
	}

	const int32 VisibleSlotLimit = FMath::Min(VisibleQuickSlotCount, ItemManagerComponent->GetSlotCount());
	for (int32 SlotIndex = 0; SlotIndex < VisibleSlotLimit; ++SlotIndex)
	{
		if (!ItemManagerComponent->IsSlotEmpty(SlotIndex))
		{
			continue;
		}

		if (ItemManagerComponent->AddItemToSlot(SlotIndex, GrantedKnifeDefinition, 1))
		{
			OutAddedSlotIndex = SlotIndex;
			return true;
		}
	}

	return ItemManagerComponent->TryAddItemToFirstEmptySlot(GrantedKnifeDefinition, 1, OutAddedSlotIndex);
}

UItemDefinition* UPersonalEventCursedSword::ResolveGrantedKnifeDefinition(const URewardDefinition* SourceRewardDefinition, const UPersonalEventCursedSwordDefinition* EventDefinition) const
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


