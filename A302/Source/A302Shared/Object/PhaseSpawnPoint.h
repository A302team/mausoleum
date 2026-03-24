// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameMode/A302GameState.h"
#include "PhaseSpawnPoint.generated.h"

/**
 * @brief 페이즈별 스폰 위치를 나타내는 마커 액터.
 * 레벨에 배치 후 PhaseTarget만 설정하면 됩니다.
 * 페이즈 전환 시 SpawnManager가 이 포인트로 플레이어를 텔레포트합니다.
 */
UCLASS()
class A302SHARED_API APhaseSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	APhaseSpawnPoint();

	/** 이 포인트가 어느 페이즈에서 사용되는지 지정합니다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	EGamePhase PhaseTarget = EGamePhase::Phase0;
};
