#include "GamePlay/Events/PersonalEvents/PersonalEventTimeKnife.h"

#include "Character/Components/ItemManagerComponent.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "GameData/Items/ItemDefinition.h"
#include "GameData/Events/PersonalEvents/PersonalEventTimeKnifeDefinition.h"
#include "GameData/Events/PersonalEvents/PersonalEventDefinition.h" // 🚩 추가됨
#include "GameData/RewardDefinition.h"
#include "GamePlay/Items/ItemTimeKnife.h"
#include "Engine/World.h"

void UPersonalEventTimeKnife::ExecuteEvent_Implementation(AMyCharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter)
	{
		return;
	}

	OwnerCharacter = InstigatorCharacter;

	AMyPlayerController* PC = Cast<AMyPlayerController>(InstigatorCharacter->GetController());
	if (!PC)
	{
		return;
	}
	
	PC->ActivePersonalEvent = this;
	
	const URewardDefinition* SourceRewardDefinition = GetRewardDefinition();
	const UPersonalEventDefinition* EventDef = Cast<UPersonalEventDefinition>(SourceRewardDefinition);
    
	if (EventDef)
	{
		PC->Client_ShowPersonalEvent(
			EventDef->ItemId, 
			EventDef->DisplayName, 
			EventDef->Description, 
			EventDef->bIsCancelable
		);
	}
	else
	{
		// 예외 처리: 만약 캐스팅에 실패했다면(이벤트 UI 데이터가 없다면) 즉시 로직 실행
		UE_LOG(LogTemp, Warning, TEXT("[PersonalEventTimeKnife] EventDef is missing, executing immediately."));
		OnEventResolved(InstigatorCharacter, true);
	}
}

void UPersonalEventTimeKnife::OnEventResolved(AMyCharacter* InstigatorCharacter, bool bIsConfirmed)
{
	// 취소가 가능한 이벤트에서 플레이어가 거절을 눌렀을 경우
	if (!bIsConfirmed)
	{
		OwnerCharacter = nullptr;
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


