// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "GameFramework/Character.h"
#include "BaseEvent.generated.h"

UCLASS(Blueprintable, BlueprintType, Abstract)
class A302SHARED_API UBaseEvent : public UObject
{
	GENERATED_BODY()
	
public:
	// 1. 이벤트 데이터
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event Info")
	FName EventID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event Info")
	FText EventTitle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event Info", meta = (MultiLine = true))
	FText EventDescription;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event Info")
	bool bIsCancelable = false;

	// 2. 이벤트 로직(블루프린트에서 오버라이드해서 사용)
	// 이벤트를 시작할 때 서버에서 호출할 함수
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Event Execution")
	void ExecuteEvent(ACharacter* InstigatorCharacter);
	virtual void ExecuteEvent_Implementation(ACharacter* InstigatorCharacter);

	// 이벤트가 끝났음을 알리는 함수 (UI 닫기, 보상 지급 완료 등)
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Event Execution")
	void OnEventResolved(ACharacter* TargetCharacter, bool bIsConfirmed);
	virtual void OnEventResolved_Implementation(ACharacter* TargetCharacter, bool bIsConfirmed);

	void InitializeRuntimeContext(ACharacter* InstigatorCharacter);

	UFUNCTION(BlueprintPure, Category = "Event Runtime")
	const FString& GetEventRoomCode() const { return EventRoomCode; }

	// 3. 시스템 필수 함수
	// UObject 기반 블루프린트에서 Delay, SpawnActor 등의 노드를 사용하기 위해 필수적인 함수
	virtual UWorld* GetWorld() const override;

private:
	UPROPERTY()
	FString EventRoomCode;
};
