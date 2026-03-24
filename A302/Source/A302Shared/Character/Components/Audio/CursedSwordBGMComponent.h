// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CursedSwordBGMComponent.generated.h"

class UAudioComponent;
class UGameBGMComponent;
class UMaliceBGMComponent;

/**
 * Added: 저주받은 검 BGM을 최우선 순위로 관리하는 컴포넌트.
 *
 * BGM 우선순위:
 *   1순위: CursedSwordBGM (저주받은 검 보유 시)
 *   2순위: MaliceBGM      (MaliceCount >= 3 이고 검 미보유 시)
 *   3순위: GameBGM        (기본 상태)
 *
 * 로컬 플레이어 전용 (IsLocalController 체크)
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302SHARED_API UCursedSwordBGMComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Added
	UCursedSwordBGMComponent();

protected:
	// Added
	virtual void BeginPlay() override;

public:
	// Added
	UPROPERTY(EditAnywhere, Category="Audio")
	USoundBase* CursedSwordBGM;

	// Added
	UPROPERTY()
	UAudioComponent* AudioComponent;

	// Added
	bool bHasCursedSword = false;

	// Added
	int32 CurrentMaliceCount = 0;

	// Added: 저주받은 검 획득 시 호출 — CursedSwordBGM 재생 (최우선)
	UFUNCTION()
	void OnCursedSwordAcquired();

	// Added: 저주받은 검 사용 후 호출 — 이전 BGM 상태로 복귀
	UFUNCTION()
	void OnCursedSwordUsed();

	// Added: Malice 수치 변경 시 호출 — 검 보유 중이면 무시
	UFUNCTION()
	void HandleMaliceState(int32 MaliceCount);

	// Added: PlayerController에서 검 보유 상태 조회용
	bool HasCursedSword() const { return bHasCursedSword; }

private:
	// Added: 현재 재생 중인 모든 BGM 정지 (CursedSword / GameBGM / MaliceBGM)
	void StopCurrentBGM();

	// Added: 기존 MaliceBGMComponent를 통해 MaliceBGM 재생
	void TriggerMaliceBGM();

	// Added: 기존 GameBGMComponent를 통해 GameBGM 재생
	void TriggerGameBGM();
};
