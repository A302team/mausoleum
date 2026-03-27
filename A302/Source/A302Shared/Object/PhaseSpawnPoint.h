// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameMode/A302GameState.h"
#include "PhaseSpawnPoint.generated.h"

/**
 * @brief 페이즈별 스폰 위치를 나타내는 마커 액터.
 * 레벨에 배치 후 PhaseTarget과 RoomCode를 설정하면 됩니다.
 * 페이즈 전환 시 SpawnManager가 이 포인트로 플레이어를 텔레포트합니다.
 *
 * RoomCode 사용 방법:
 *   - 비워두면(기존 방식): 이 스폰 포인트의 월드 X좌표로 룸을 판단합니다.
 *   - 직접 입력(권장): "A302", "B", "C" 등 방 코드를 명시하면 X좌표 계산 없이 해당 룸에만 사용됩니다.
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

	/**
	 * 이 스폰 포인트가 속한 방 코드. (대소문자 무시, 공백 자동 제거)
	 * 비어 있으면 월드 X좌표 기반 룸 범위 필터링으로 Fallback됩니다.
	 * 권장: 방 코드를 직접 입력하세요. (예: "A302", "B", "ROOM_C")
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Phase")
	FString RoomCode;
};
