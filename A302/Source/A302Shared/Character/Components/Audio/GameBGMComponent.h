// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameBGMComponent.generated.h"

class UAudioComponent;
class UMaliceBGMComponent; // Added

/**
 * 게임 기본 BGM을 관리하는 컴포넌트.
 * - 기본 상태: GameBGM 재생
 * - MaliceCount >= 3: GameBGM 정지 → MaliceBGMComponent 호출
 * - MaliceCount < 3: MaliceBGMComponent 호출 → GameBGM 재개
 * 로컬 플레이어 전용 (IsLocalController 체크)
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302SHARED_API UGameBGMComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGameBGMComponent();

protected:
	virtual void BeginPlay() override;

public:
	// Added
	UPROPERTY(EditAnywhere, Category="Audio")
	USoundBase* GameBGM;

	// Added
	UPROPERTY()
	UAudioComponent* GameBGMAudioComponent;

	// Added
	UPROPERTY()
	UMaliceBGMComponent* MaliceBGMComponentRef;

	// Added
	bool bIsMaliceState = false;

	// Added
	UFUNCTION()
	void HandleMaliceState(int32 MaliceCount);

	// Added
	void PlayGameBGM();

	// Added
	void StopGameBGM();
};
