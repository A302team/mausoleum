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

namespace
{
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
		TArray<FText> Choices;
		if (EventDef->bIsCancelable)
		{
			Choices.Add(FText::FromString(TEXT("거절")));
			Choices.Add(FText::FromString(TEXT("수락")));
		}
		else
		{
			// 거절 불가능한 이벤트라도, 인덱스 1(수락)을 맞추기 위해 0번을 더미로 넣습니다.
			// (블루프린트 위젯에서 텍스트가 비어있으면 숨기거나, 무시하도록 처리 가능)
			Choices.Add(FText::FromString(TEXT(""))); 
			Choices.Add(FText::FromString(TEXT("확인"))); 
		}

		PC->Client_ShowPersonalEvent(
		   EventDef->ItemId, 
		   EventDef->DisplayName, 
		   EventDef->Description, 
		   Choices
		);
	}
	else
	{
		// 예외 처리: 만약 캐스팅에 실패했다면(이벤트 UI 데이터가 없다면) 즉시 로직 실행
		UE_LOG(LogTemp, Warning, TEXT("[PersonalEventTimeKnife] EventDef is missing, executing immediately."));
		OnEventResolved(InstigatorCharacter, true);
	}
}

void UPersonalEventCursedSword::OnEventResolved(AMyCharacter* InstigatorCharacter, bool bIsConfirmed)
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

	if (AMyPlayerController* PC = Cast<AMyPlayerController>(InstigatorCharacter->GetController()))
	{
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


