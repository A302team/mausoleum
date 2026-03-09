// Fill out your copyright notice in the Description page of Project Settings.

#include "GamePlay/Events/BaseEvent.h"
#include "Engine/World.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"

void UBaseEvent::ExecuteEvent_Implementation(AMyCharacter* InstigatorCharacter)
{	
	if (!InstigatorCharacter) return;

	// C++ 기본 동작: 플레이어 컨트롤러를 찾아 UI를 띄우라고 지시합니다.
	if (AMyPlayerController* PC = Cast<AMyPlayerController>(InstigatorCharacter->GetController()))
	{
		// 서버 측 컨트롤러에 현재 진행 중인 이벤트를 등록합니다.
		PC->ActivePersonalEvent = this;
        
		// 클라이언트 RPC를 호출하여 화면에 띄웁니다.
		PC->Client_ShowPersonalEvent(EventID, EventTitle, EventDescription, bIsCancelable);
	}
	UE_LOG(LogTemp, Warning, TEXT("[Event] %s 실행됨! (C++ 기본 구현)"), *EventID.ToString());
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