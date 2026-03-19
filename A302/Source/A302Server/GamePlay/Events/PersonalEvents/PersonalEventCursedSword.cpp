#include "GamePlay/Events/PersonalEvents/PersonalEventCursedSword.h"

#include "Character/Components/Inventory/ItemManagerComponent.h"
#include "GameData/Events/PersonalEvents/Equipment/PersonalEventCursedSwordDefinition.h"
#include "GameData/Events/PersonalEvents/PersonalEventDefinition.h"
#include "GameData/Items/ItemDefinition.h"
#include "GameData/RewardDefinition.h"
#include "GamePlay/Items/BaseItem.h"
#include "GamePlay/Items/ItemTimeKnife.h"
#include "Engine/World.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"

namespace
{
	constexpr int32 VisibleQuickSlotCount = 5;
}

void UPersonalEventCursedSword::ExecuteEvent_Implementation(ACharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter)
	{
		const int32 SafeSeconds = FMath::Max(0, FMath::CeilToInt(RemainingSeconds));
		return FText::FromString(FString::Printf(TEXT("%d초 안에 사용하지 않을 시 사망합니다."), SafeSeconds));
	}

	FText ResolveCursedSwordTitle(const UItemDefinition* GrantedKnifeDefinition, const UPersonalEventCursedSwordDefinition* EventDef)
	{
		if (GrantedKnifeDefinition && !GrantedKnifeDefinition->DisplayName.IsEmpty())
		{
			return GrantedKnifeDefinition->DisplayName;
		}

		if (EventDef && !EventDef->DisplayName.IsEmpty())
		return;
	}

	OwnerCharacter = InstigatorCharacter;

	AMyPlayerController* ClientEventBridge = Cast<AMyPlayerController>(InstigatorCharacter->GetController());
	if (!ClientEventBridge)
	{
		return;
	}

	ClientEventBridge->SetActivePersonalEvent(this);

	const URewardDefinition* SourceRewardDefinition = GetRewardDefinition();
	const UPersonalEventDefinition* EventDef = Cast<UPersonalEventDefinition>(SourceRewardDefinition);

	if (EventDef)
	{
		TArray<FText> Choices;
		if (EventDef->bIsCancelable)
		{
			Choices.Add(FText::FromString(TEXT("거절")));
			Choices.Add(FText::FromString(TEXT("수락")));
		}
		else
		{
			Choices.Add(FText::FromString(TEXT("")));
			Choices.Add(FText::FromString(TEXT("확인")));
		}

		ClientEventBridge->ShowPersonalEvent(
			EventDef->ItemId,
			EventDef->DisplayName,
			EventDef->Description,
			Choices
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PersonalEventTimeKnife] EventDef is missing, executing immediately."));
		OnEventResolved(InstigatorCharacter, true);
	}
}

void UPersonalEventCursedSword::OnEventResolved(ACharacter* InstigatorCharacter, bool bIsConfirmed)
{
	if (!bIsConfirmed || !InstigatorCharacter || !InstigatorCharacter->HasAuthority() || bIsActive)
	{
		if (!bIsConfirmed)
		{
			OwnerCharacter = nullptr;
		}
		return;
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
		return;
	}

	int32 AddedSlotIndex = INDEX_NONE;
	if (!TryGrantKnifeToPreferredSlot(InstigatorCharacter, GrantedKnifeDefinition, AddedSlotIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PersonalEventTimeKnife] Failed to grant timed knife: add failed or slot full."));
		OwnerCharacter = nullptr;
		return;
	}

	if (UClass* LogicClass = GrantedKnifeDefinition->ResolveRewardLogicClass())
	{
		if (LogicClass->IsChildOf(UBaseItem::StaticClass()))
		{
			if (const UBaseItem* BaseItemLogic = Cast<UBaseItem>(LogicClass->GetDefaultObject()))
			{
				BaseItemLogic->OnItemAcquired(InstigatorCharacter);
			}
		}
	}

	GrantedItemId = GrantedKnifeDefinition->ItemId;
	GrantedSlotIndex = AddedSlotIndex;
	bIsActive = true;

	if (AMyCharacter* CharacterBridge = Cast<AMyCharacter>(InstigatorCharacter))
	{
		CharacterBridge->RegisterTimedKillEvent(this);
	}

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
}

void UPersonalEventCursedSword::NotifyTimedKillConfirmed()
{
	if (!bIsActive)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[PersonalEventTimeKnife] Objective success."));
	StopCountdown(true);
}

void UPersonalEventCursedSword::CancelTimedKillCountdown()
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

	ACharacter* Character = OwnerCharacter.Get();
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
	if (AMyCharacter* CharacterBridge = Cast<AMyCharacter>(Character))
	{
		CharacterBridge->ForceDeathByPersonalEvent();
	}
}

void UPersonalEventCursedSword::RefreshTimerUI() const
{
	ACharacter* Character = OwnerCharacter.Get();
	if (!Character)
	{
		return;
	}

	if (AMyPlayerController* ClientEventBridge = Cast<AMyPlayerController>(Character->GetController()))
	{
		ClientEventBridge->UpdateItemTimer(RemainingSeconds);
		ClientEventBridge->SetItemTimerVisibleForClient(true);
	}
}

void UPersonalEventCursedSword::StopCountdown(bool bHideTimer)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CountdownTimerHandle);
	}

	ACharacter* Character = OwnerCharacter.Get();
	if (Character)
	{
		if (AMyCharacter* CharacterBridge = Cast<AMyCharacter>(Character))
		{
			CharacterBridge->ClearTimedKillEvent(this);
		}

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
			if (AMyPlayerController* ClientEventBridge = Cast<AMyPlayerController>(Character->GetController()))
			{
				ClientEventBridge->SetItemTimerVisibleForClient(false);
			}
		}
	}

	OwnerCharacter = nullptr;
	GrantedItemId = NAME_None;
	GrantedSlotIndex = INDEX_NONE;
	RemainingSeconds = 0.0f;
	bIsActive = false;
}

bool UPersonalEventCursedSword::TryGrantKnifeToPreferredSlot(ACharacter* InstigatorCharacter, UItemDefinition* GrantedKnifeDefinition, int32& OutAddedSlotIndex) const
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

	if (EventDefinition)
	{
		UItemDefinition* GrantedDefinition = EventDefinition->Payload.GrantedItemDefinition.Get();
		if (IsValidTimedKnifeItem(GrantedDefinition))
		{
			return GrantedDefinition;
		}
	}

	UItemDefinition* SourceAsItemDefinition = Cast<UItemDefinition>(const_cast<URewardDefinition*>(SourceRewardDefinition));
	if (IsValidTimedKnifeItem(SourceAsItemDefinition))
	{
		return SourceAsItemDefinition;
	}

	return nullptr;
}
