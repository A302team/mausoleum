// Fill out your copyright notice in the Description page of Project Settings.

#include "GamePlay/Events/BaseEvent.h"
#include "Engine/World.h"
#include "GameMode/A302PlayerState.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"
#include "Interface/A302ClientEventBridge.h"

void UBaseEvent::InitializeRuntimeContext(ACharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter)
	{
		EventRoomCode.Reset();
		return;
	}

	if (const AA302PlayerState* InstigatorState = InstigatorCharacter->GetPlayerState<AA302PlayerState>())
	{
		EventRoomCode = InstigatorState->GetRoomCode();
	}
	else
	{
		EventRoomCode.Reset();
	}
}

void UBaseEvent::ExecuteEvent_Implementation(ACharacter* InstigatorCharacter)
{
	UE_LOG(LogTemp, Warning, TEXT("[Event] ExecuteEvent_Implementation 진입 완료."));

	if (!InstigatorCharacter) return;
	InitializeRuntimeContext(InstigatorCharacter);

	if (IA302ClientEventBridge* ClientEventBridge = Cast<IA302ClientEventBridge>(InstigatorCharacter->GetController()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Event] 클라이언트 브리지 찾음! UI 출력 RPC 호출 시도!"));
		if (UBasePersonalEvent* PersonalEvent = Cast<UBasePersonalEvent>(this))
		{
			ClientEventBridge->SetActivePersonalEvent(PersonalEvent);
		}

		TArray<FText> Choices;
		if (bIsCancelable)
		{
			Choices.Add(FText::FromString(TEXT("취소")));
			Choices.Add(FText::FromString(TEXT("확인")));
		}
		else
		{
			Choices.Add(FText::FromString(TEXT("")));
			Choices.Add(FText::FromString(TEXT("확인")));
		}

		ClientEventBridge->ShowPersonalEvent(
			EventID,
			EventTitle,
			EventDescription,
			Choices
		);
	}
}

void UBaseEvent::OnEventResolved_Implementation(ACharacter* TargetCharacter, bool bIsConfirmed)
{
	// C++ 기본 구현부 (보통은 비워두고 블루프린트에서 노드로 구현합니다)
	UE_LOG(LogTemp, Warning, TEXT("[Event] %s 이벤트가 확인되었습니다! (보상 지급 로직 실행)"), *EventID.ToString());
}

UWorld* UBaseEvent::GetWorld() const
{
	if (HasAnyFlags(RF_ClassDefaultObject)) return nullptr;
	if (GetOuter()) return GetOuter()->GetWorld();
	return nullptr;
}
