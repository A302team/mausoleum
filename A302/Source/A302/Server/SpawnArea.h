// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TriggerBox.h"
#include "SpawnArea.generated.h"


/**
 * @brief 스폰 영역을 나타내는 클래스. ATriggerBox를 상속받아 특정 영역 내에서 플레이어가 스폰될 수 있도록 설정.
 * @note 이 클래스는 스폰 영역을 정의하는 데 사용되며, 게임 모드에서 플레이어의 시작 위치를 결정할 때 활용될 수 있습니다.
 * @author 홍윤표
 * @date 2026-02-26
 */
UCLASS()
class A302_API ASpawnArea : public ATriggerBox
{
	GENERATED_BODY()
public:
	// 생성자
	ASpawnArea();
	UPROPERTY(EditAnywhere, Category = "Spawn")
	int32 TargetStage = 0; // 스폰 영역이 활성화되는 스테이지 번호

	FVector GetRandomPointInBox() const;
};
