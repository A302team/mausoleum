#pragma once

#include "CoreMinimal.h"
#include "GameMode/A302GameState.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "A302ItemSpawnSubsystem.generated.h"

class UA302ItemSpawnOrchestrator;
class UItemSpawnPolicy;

UCLASS(Config = Server)
class A302SERVER_API UA302ItemSpawnSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void HandleRoomLevelReady(const FString& RoomCode, EGamePhase CurrentPhase);
	void HandleRoomPhaseChanged(const FString& RoomCode, EGamePhase NewPhase);
	void ResetRoom(const FString& RoomCode);

private:
	UPROPERTY(Config, EditAnywhere, Category = "Item Spawn")
	TSoftObjectPtr<UItemSpawnPolicy> DefaultSpawnPolicy;

	UPROPERTY(Config, EditAnywhere, Category = "Item Spawn", meta = (ClampMin = "1"))
	int32 SpawnRetryPerItem = 5;

	UPROPERTY(Config, EditAnywhere, Category = "Item Spawn", meta = (ClampMin = "0.1"))
	float RoomMatchToleranceRatio = 0.45f;

	UPROPERTY(Config, EditAnywhere, Category = "Item Spawn")
	bool bClearPreviousPhaseItems = true;

	UPROPERTY(Config, EditAnywhere, Category = "Item Spawn|Log")
	bool bLogItemSpawns = true;

	UPROPERTY(Transient)
	TObjectPtr<UA302ItemSpawnOrchestrator> Orchestrator = nullptr;
};
