#include "GamePlay/Events/PersonalEvents/PersonalEventCursedSword.h"

#include "Character/Components/ItemManagerComponent.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "GameData/Items/ItemDefinition.h"
#include "GameData/Events/PersonalEvents/PersonalEventCursedSwordDefinition.h"
#include "GameData/Events/PersonalEvents/PersonalEventDefinition.h" 
#include "GameData/RewardDefinition.h"
#include "GamePlay/Items/ItemTimeKnife.h"
#include "Engine/World.h"

void UPersonalEventCursedSword::ExecuteEvent_Implementation(AMyCharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter) return;
	OwnerCharacter = InstigatorCharacter;

	AMyPlayerController* PC = Cast<AMyPlayerController>(InstigatorCharacter->GetController());
	if (!PC) return;
    
	PC->ActivePersonalEvent = this;
    
	TArray<FText> Choices;
	Choices.Add(FText::FromString(TEXT("확인")));
	
	PC->Client_ShowPersonalEvent(
	   FName("CursedSword"), 
	   FText::FromString(TEXT("저주받은 검")), 
	   FText::FromString(TEXT("피를 갈망하는 저주받은 검을 습득했습니다.\n제한 시간 내에 누군가를 공격하지 않으면 검 끝이 당신을 향할 것입니다.")), 
	   Choices
	);
}

void UPersonalEventCursedSword::OnEventResolved_Implementation(AMyCharacter* InstigatorCharacter, int32 ChoiceIndex)
{
	if (ChoiceIndex != 0) return;
	
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
		   &UPersonalEventCursedSword::HandleCountdownTick,
		   1.0f,
		   true
		);
	}

	UE_LOG(LogTemp, Log, TEXT("[PersonalEventTimeKnife] Countdown started: %.0fs"), RemainingSeconds);
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
		PlayerController->UpdateItemTimerText(RemainingSeconds);
		PlayerController->SetItemTimerVisible(true);
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


