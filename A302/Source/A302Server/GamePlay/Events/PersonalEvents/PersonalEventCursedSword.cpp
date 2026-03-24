#include "GamePlay/Events/PersonalEvents/PersonalEventCursedSword.h"

#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "Character/Components/Audio/CursedSwordBGMComponent.h" // Added
#include "Character/Components/Interaction/CharacterRewardComponent.h"
#include "Character/Components/Inventory/ItemManagerComponent.h"
#include "Character/Components/PlayerEventComponent.h"
#include "Engine/World.h"
#include "GameData/Events/PersonalEvents/Equipment/PersonalEventCursedSwordDefinition.h"
#include "GameData/Items/ItemDefinition.h"
#include "GameData/RewardDefinition.h"
#include "GamePlay/Items/BaseItem.h"
#include "GamePlay/Items/ItemCursedSword.h"

namespace
{
	constexpr int32 VisibleQuickSlotCount = 6;

	FText BuildCursedSwordCountdownContext(float RemainingSeconds)
	{
		const int32 SafeSeconds = FMath::Max(0, FMath::CeilToInt(RemainingSeconds));
		return FText::FromString(FString::Printf(TEXT("피를 갈망하는 저주받은 검을 습득했습니다.\n%d초 내에 누군가를 공격하지 않으면 검 끝이 당신을 향할 것입니다."), SafeSeconds));
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

	PlayerController->Client_ShowTitleCard(
		FText::FromString(TEXT("저주받은 검")),
		FText::FromString(TEXT("피를 갈망하는 저주받은 검을 습득했습니다.\n제한 시간 내에 누군가를 공격하지 않으면 검 끝이 당신을 향할 것입니다.")),
		5.0f
	);

	OnEventResolved(InstigatorCharacter, 0);
}

void UPersonalEventCursedSword::OnEventResolved(ACharacter* InstigatorCharacter, int32 ChoiceIndex)
{
	if (ChoiceIndex != 0 || !InstigatorCharacter || !InstigatorCharacter->HasAuthority() || bIsActive)
	{
		if (ChoiceIndex != 0)
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
	bCountdownPaused = false;

	// Added: 저주받은 검 BGM 재생 시작
	if (AMyPlayerController* PlayerController = Cast<AMyPlayerController>(InstigatorCharacter->GetController()))
	{
		if (PlayerController->CursedSwordBGMComp)
		{
			PlayerController->CursedSwordBGMComp->OnCursedSwordAcquired();
		}
	}
	// End Added

	if (AMyPlayerController* PlayerController = Cast<AMyPlayerController>(InstigatorCharacter->GetController()))
	{
		PlayerController->Client_ShowTitleCard(
			ResolveCursedSwordTitle(GrantedCursedSwordDefinition, EventDef),
			BuildCursedSwordCountdownContext(RemainingSeconds),
			1.5f
		);
	}

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

void UPersonalEventCursedSword::PauseTimedKillCountdown()
{
	if (!bIsActive || bCountdownPaused)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().PauseTimer(CountdownTimerHandle);
	}

	bCountdownPaused = true;
}

void UPersonalEventCursedSword::ResumeTimedKillCountdown()
{
	if (!bIsActive || !bCountdownPaused)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().UnPauseTimer(CountdownTimerHandle);
	}

	bCountdownPaused = false;
	RefreshTimerUI();
}

void UPersonalEventCursedSword::BeginDestroy()
{
	StopCountdown(true);
	Super::BeginDestroy();
}

void UPersonalEventCursedSword::HandleCountdownTick()
{
	if (!bIsActive || bCountdownPaused)
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

	if (AMyPlayerController* ClientEventBridge = Cast<AMyPlayerController>(Character->GetController()))
	{
		// 이전에 저장할 수 없었던 Title을 로컬라이징 없이 재사용
		ClientEventBridge->Client_ShowTitleCard(
			FText::FromString(TEXT("저주받은 검")),
			BuildCursedSwordCountdownContext(RemainingSeconds),
			1.5f
		);
	}

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

			// Added: 저주받은 검 제거 시 BGM 복귀
			if (AMyPlayerController* PlayerController = Cast<AMyPlayerController>(Character->GetController()))
			{
				if (PlayerController->CursedSwordBGMComp)
				{
					PlayerController->CursedSwordBGMComp->OnCursedSwordUsed();
				}
			}
			// End Added
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
	bCountdownPaused = false;
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
