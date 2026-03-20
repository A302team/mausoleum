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
	// 1. 이벤트 기본 정보
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event Info")
	FName EventID;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event Info")
	FText EventTitle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Event Info", meta = (MultiLine = true))
	FText EventDescription;

	// 2. 이벤트 실행 함수
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Event Execution")
	void ExecuteEvent(ACharacter* InstigatorCharacter);
	virtual void ExecuteEvent_Implementation(ACharacter* InstigatorCharacter);

	// 이벤트 종료 후 호출되는 콜백
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Event Execution")
	void OnEventResolved(ACharacter* TargetCharacter, bool bIsConfirmed);
	virtual void OnEventResolved_Implementation(ACharacter* TargetCharacter, bool bIsConfirmed);

	void InitializeRuntimeContext(ACharacter* InstigatorCharacter);

	UFUNCTION(BlueprintPure, Category = "Event Runtime")
	const FString& GetEventRoomCode() const { return EventRoomCode; }

	// 3. 월드 컨텍스트 제공
	virtual UWorld* GetWorld() const override;

private:
	UPROPERTY()
	FString EventRoomCode;
};
