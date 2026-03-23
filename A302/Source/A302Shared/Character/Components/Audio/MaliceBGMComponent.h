// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MaliceBGMComponent.generated.h"

class UAudioComponent;

/**
 * MaliceCount에 따라 BGM을 재생/정지하는 컴포넌트.
 * MaliceCount >= 3 이면 BGM 재생, 미만이면 정지.
 * 중복 재생 방지 및 bAutoDestroy = false로 루핑 관리.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class A302SHARED_API UMaliceBGMComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMaliceBGMComponent();

	UFUNCTION()
	void HandleMaliceBGM(int32 MaliceCount);

private:
	UPROPERTY(EditAnywhere, Category = "Audio")
	USoundBase* MaliceBGM = nullptr;

	UPROPERTY()
	UAudioComponent* BGMAudioComponent = nullptr;

	bool bIsMaliceBGMPlaying = false;
};
