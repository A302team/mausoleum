// Fill out your copyright notice in the Description page of Project Settings.

#include "GamePlay/Events/BaseEvent.h"
#include "Engine/World.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"

void UBaseEvent::ExecuteEvent_Implementation(AMyCharacter* InstigatorCharacter)
{
	UE_LOG(LogTemp, Warning, TEXT("[Event] ExecuteEvent_Implementation 진입 완료."));

	if (!InstigatorCharacter) return;

	AMyPlayerController* PC = Cast<AMyPlayerController>(InstigatorCharacter->GetController());
	if (PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Event] 플레이어 컨트롤러 찾음! UI 출력 RPC 호출 시도!"));
		PC->ActivePersonalEvent = this;
        
		// 🚩 [핵심 수정] 여기서 실제로 PC의 함수를 호출해야 합니다!
		// UBaseEvent 헤더(.h)에 만들어두신 변수들을 집어넣어 줍니다.
		// (만약 변수 이름이 다르다면 본인의 변수 이름으로 맞춰주세요)
		PC->Client_ShowPersonalEvent(
			this->EventID, 
			this->EventTitle, 
			this->EventDescription, 
			this->bIsCancelable
		);
	}
}

void UBaseEvent::OnEventResolved_Implementation(AMyCharacter* TargetCharacter, bool bIsConfirmed)
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