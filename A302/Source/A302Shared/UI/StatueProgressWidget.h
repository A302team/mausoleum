// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StatueProgressWidget.generated.h"

class UProgressBar;
class AStatueInteractable;

/**
 * 전역 동기화된 석상 게이지를 시각적으로 보여주는 네이티브 위젯 베이스 클래스입니다.
 */
UCLASS(Abstract)
class A302SHARED_API UStatueProgressWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// UI 표시 시작 및 추적 대상 세팅
	UFUNCTION()
	void StartTracking(AActor* TargetActor);

	// UI 표시 종료
	UFUNCTION()
	void StopTracking();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	// 위젯 블루프린트에서 동일한 이름으로 생성된 ProgressBar를 자동 바인딩합니다.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UProgressBar* ProgressBar_Progress;

private:
	UPROPERTY()
	AStatueInteractable* CurrentStatue = nullptr;

	UPROPERTY()
	class UInteractComponent* CachedInteractComponent = nullptr;

	bool TryBindToInteractComponent();
};
