// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SpawnManager.generated.h"

/**
 * @brief 게임 내에서 플레이어의 스폰 위치를 관리하는 클래스. 
 * AActor를 상속받아 게임 월드에 배치되며, 특정 스테이지에 따라 안전한 스폰 위치를 반환하는 기능을 제공합니다.
 * @author 홍윤표
 * @date 2026-02-26
 */
UCLASS()
class A302SERVER_API ASpawnManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASpawnManager();

    static ASpawnManager* FindOrSpawn(class UWorld* World);
    void QueueSpawnAndPossessPlayer(class APlayerController* PlayerController, TSubclassOf<class ACharacter> CharacterClass, int32 StageNum, const FString& RoomCode, float WarmupSeconds);
    bool SpawnAndPossessPlayer(class APlayerController* PlayerController, TSubclassOf<class ACharacter> CharacterClass, int32 StageNum, const FString& RoomCode);
	FTransform GetRandomPlayerSpawnTransform(int32 Stage, const FString& RoomCode) const;
private:
    bool IsLocationSafe(const FVector& Location) const;
	FVector GetRoomOffset(const FString& RoomCode) const;
    bool IsSpawnAreaInRoomSpace(const class ASpawnArea* SpawnArea, const FString& RoomCode) const;
};
