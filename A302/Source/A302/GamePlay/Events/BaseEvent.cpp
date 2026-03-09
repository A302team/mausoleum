// Fill out your copyright notice in the Description page of Project Settings.

#include "BaseEvent.h"
#include "Engine/World.h"
#include "Character/MyCharacter.h"

void UBaseEvent::ExecuteEvent_Implementation(AMyCharacter* InstigatorCharacter)
{
	UE_LOG(LogTemp, Warning, TEXT("[Event] %s 실행됨! (C++ 기본 구현)"), *EventID.ToString());
}

void UBaseEvent::FinishEvent(bool bWasSuccessful)
{
	// TODO: GameMode나 GameState에 이벤트 종료를 알리고, 원래 입력 상태(IMC)로 복구하는 처리
	UE_LOG(LogTemp, Log, TEXT("[Event] %s 종료. 성공 여부: %d"), *EventID.ToString(), bWasSuccessful);
}

UWorld* UBaseEvent::GetWorld() const
{
	// 에디터의 Class Default Object(CDO)인 경우 예외 처리
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}
    
	// 이 오브젝트를 생성한 부모(보통 GameMode)의 World를 반환
	if (GetOuter())
	{
		return GetOuter()->GetWorld();
	}

	return nullptr;
}