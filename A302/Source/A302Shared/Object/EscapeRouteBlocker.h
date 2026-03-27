// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EscapeRouteBlocker.generated.h"

UCLASS()
class A302SHARED_API AEscapeRouteBlocker : public AActor
{
	GENERATED_BODY()

public:
	AEscapeRouteBlocker();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USceneComponent* DefaultRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UBoxComponent* BlockerBox;

	// 서서히 사라지는 연출 시간 (기본 3초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escape")
	float FadeDuration = 3.0f;

	// 수동 호출: 서버에서 직접 호출해 탈출로를 개방
	void OpenEscapeRoute();

private:
	// 시간적으로 서서히 사라지는 연출(스케일 다운)을 위한 상태값
	bool bIsEscapeOpened = false;
	float CurrentFadeTime = 0.0f;
	FVector InitialScale;
};
