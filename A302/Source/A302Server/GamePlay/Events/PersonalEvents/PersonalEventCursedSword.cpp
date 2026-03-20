#include "GamePlay/Events/PersonalEvents/PersonalEventCursedSword.h"

#include "Character/Components/Inventory/ItemManagerComponent.h"
#include "GameData/Events/PersonalEvents/Equipment/PersonalEventCursedSwordDefinition.h"
#include "GameData/Events/PersonalEvents/PersonalEventDefinition.h"
#include "GameData/Items/ItemDefinition.h"
#include "GameData/RewardDefinition.h"
#include "GamePlay/Items/BaseItem.h"
#include "GamePlay/Items/ItemCursedSword.h"
#include "Engine/World.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "Character/Components/PlayerEventComponent.h"
#include "Character/Components/Interaction/CharacterRewardComponent.h"

namespace
{
	constexpr int32 VisibleQuickSlotCount = 5;

	FText BuildCursedSwordCountdownContext(float RemainingSeconds)
	{
		const int32 SafeSeconds = FMath::Max(0, FMath::CeilToInt(RemainingSeconds));
		return FText::FromString(FString::Printf(TEXT("%d초 안에 사용하지 않을 시 사망합니다."), SafeSeconds));
	}

	FText ResolveCursedSwordTitle(const UItemDefinition* GrantedCursedSwordDefinition, const UPersonalEventCursedSwordDefinition* EventDef)
	{
		if (GrantedCursedSwordDefinition && !GrantedCursedSwordDefinition->DisplayName.IsEmpty())
		{
			return GrantedCursedSwordDefinition->DisplayName;
		}

		if (EventDef && !EventDef->DisplayName.IsEmpty())
		{
			return EventDef->DisplayName;
		}

		return FText::FromString(TEXT("저주받은 검 획득"));
	}
}

void UPersonalEventCursedSword::ExecuteEvent_Implementation(ACharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter || !InstigatorCharacter->HasAuthority())
	{
		return;
	}

	OwnerCharacter = InstigatorCharacter;

	AMyPlayerController* PlayerController = Cast<AMyPlayerController>(InstigatorCharacter->GetController());
	if (!PlayerController)
	{
		return;
	}

	UPlayerEventComponent* EventComp = PlayerController->GetPlayerEventComponent();
	if (!EventComp)
	{
		return;
	}

	EventComp->SetActivePersonalEvent(this);

	// Personal event UI can fail to load in broken local/editor setups.
	// Keep cursed sword flow alive by resolving immediately on server.
	UE_LOG(LogTemp, Warning, TEXT("[PersonalEventCursedSword] UI fallback: resolving immediately. instigator=%s reward=%s"), *GetNameSafe(InstigatorCharacter), *GetNameSafe(GetRewardDefinition()));
	OnEventResolved(InstigatorCharacter, true);
	return;

#if 0

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

		EventComp->ShowPersonalEvent(
			EventDef->ItemId,
			EventDef->DisplayName,
			EventDef->Description,
			Choices
		);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PersonalEventCursedSword] EventDef is missing, executing immediately."));
		OnEventResolved(InstigatorCharacter, true);
	}
#endif
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
	RemainingSeconds = EventDef ? FMath::Max(1.0f, EventDef->Payload.TimedKillDuration) : 10.0f;

	UItemDefinition* GrantedCursedSwordDefinition = ResolveGrantedCursedSwordDefinition(SourceRewardDefinition, EventDef);
	if (!GrantedCursedSwordDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PersonalEventCursedSword] Granted cursed sword definition is missing."));
		OwnerCharacter = nullptr;
		return;
	}

	int32 AddedSlotIndex = INDEX_NONE;
	if (!TryGrantCursedSwordToPreferredSlot(InstigatorCharacter, GrantedCursedSwordDefinition, AddedSlotIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PersonalEventCursedSword] Failed to grant cursed sword: add failed or slot full."));
		OwnerCharacter = nullptr;
		return;
	}

	if (UClass* LogicClass = GrantedCursedSwordDefinition->ResolveRewardLogicClass())
	{
		if (LogicClass->IsChildOf(UBaseItem::StaticClass()))
		{
			if (const UBaseItem* BaseItemLogic = Cast<UBaseItem>(LogicClass->GetDefaultObject()))
			{
				BaseItemLogic->OnItemAcquired(InstigatorCharacter);
			}
		}
	}

	if (UCharacterRewardComponent* RewardComponent = InstigatorCharacter->FindComponentByClass<UCharacterRewardComponent>())
	{
		const bool bNeedsClientMirrorGrant =
			!InstigatorCharacter->IsLocallyControlled() &&
			Cast<AMyPlayerController>(InstigatorCharacter->GetController()) != nullptr;

		if (bNeedsClientMirrorGrant)
		{
			RewardComponent->Client_GrantInteractionReward(GrantedCursedSwordDefinition);
		}
	}

	GrantedItemId = GrantedCursedSwordDefinition->ItemId;
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

}

void UPersonalEventCursedSword::NotifyTimedKillConfirmed()
{
	if (!bIsActive)
	{
		return;
	}

	StopCountdown(true);
}

void UPersonalEventCursedSword::CancelTimedKillCountdown()
{
	if (!bIsActive)
	{
		return;
	}

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

bool UPersonalEventCursedSword::TryGrantCursedSwordToPreferredSlot(ACharacter* InstigatorCharacter, UItemDefinition* GrantedCursedSwordDefinition, int32& OutAddedSlotIndex) const
{
	OutAddedSlotIndex = INDEX_NONE;
	if (!InstigatorCharacter || !GrantedCursedSwordDefinition)
	{
		return false;
	}

	UItemManagerComponent* ItemManagerComponent = InstigatorCharacter->FindComponentByClass<UItemManagerComponent>();
	if (!ItemManagerComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PersonalEventCursedSword] Grant failed: ItemManagerComponent missing on %s"), *GetNameSafe(InstigatorCharacter));
		return false;
	}

	const int32 VisibleSlotLimit = FMath::Min(VisibleQuickSlotCount, ItemManagerComponent->GetSlotCount());
	for (int32 SlotIndex = 0; SlotIndex < VisibleSlotLimit; ++SlotIndex)
	{
		if (!ItemManagerComponent->IsSlotEmpty(SlotIndex))
		{
			continue;
		}

		if (ItemManagerComponent->AddItemToSlot(SlotIndex, GrantedCursedSwordDefinition, 1))
		{
			OutAddedSlotIndex = SlotIndex;
			return true;
		}
	}

	return ItemManagerComponent->TryAddItemToFirstEmptySlot(GrantedCursedSwordDefinition, 1, OutAddedSlotIndex);
}

UItemDefinition* UPersonalEventCursedSword::ResolveGrantedCursedSwordDefinition(const URewardDefinition* SourceRewardDefinition, const UPersonalEventCursedSwordDefinition* EventDefinition) const
{
	auto IsValidCursedSwordItem = [](const UItemDefinition* Candidate)
	{
		if (!Candidate)
		{
			return false;
		}

		UClass* CandidateLogicClass = Candidate->ResolveRewardLogicClass();
		return CandidateLogicClass && CandidateLogicClass->IsChildOf(UItemCursedSword::StaticClass());
	};

	if (EventDefinition)
	{
		UItemDefinition* GrantedDefinition = EventDefinition->Payload.GrantedItemDefinition.Get();
		if (IsValidCursedSwordItem(GrantedDefinition))
		{
			return GrantedDefinition;
		}
	}

	UItemDefinition* SourceAsItemDefinition = Cast<UItemDefinition>(const_cast<URewardDefinition*>(SourceRewardDefinition));
	if (IsValidCursedSwordItem(SourceAsItemDefinition))
	{
		return SourceAsItemDefinition;
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[PersonalEventCursedSword] Failed to resolve granted cursed sword definition. source=%s sourceLogic=%s"),
		*GetNameSafe(SourceAsItemDefinition),
		*GetNameSafe(SourceAsItemDefinition ? SourceAsItemDefinition->ResolveRewardLogicClass() : nullptr)
	);

	return nullptr;
}
