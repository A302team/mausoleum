#include "Subsystem/A302ItemSpawnSubsystem.h"

#include "Engine/GameInstance.h"
#include "GameData/Spawn/ItemSpawnPolicy.h"
#include "ItemSpawn/A302ItemSpawnOrchestrator.h"
#include "UObject/SoftObjectPath.h"

void UA302ItemSpawnSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadConfig();

	if (DefaultSpawnPolicy.IsNull())
	{
		static const TCHAR* FallbackPolicyPath = TEXT("/Game/WorkSpace/GameData/spawn/DA_ItemSpawnPolicy.DA_ItemSpawnPolicy");
		DefaultSpawnPolicy = TSoftObjectPtr<UItemSpawnPolicy>(FSoftObjectPath(FallbackPolicyPath));
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[ItemSpawn] DefaultSpawnPolicy is empty from config. Applying fallback path: %s"),
			FallbackPolicyPath
		);
	}

	if (!DefaultSpawnPolicy.IsNull())
	{
		UE_LOG(LogTemp, Log, TEXT("[ItemSpawn] Effective DefaultSpawnPolicy: %s"), *DefaultSpawnPolicy.ToString());
	}

	if (!Orchestrator)
	{
		Orchestrator = NewObject<UA302ItemSpawnOrchestrator>(this);
	}

	if (Orchestrator)
	{
		Orchestrator->Initialize(
			GetGameInstance(),
			DefaultSpawnPolicy,
			SpawnRetryPerItem,
			RoomMatchToleranceRatio,
			bClearPreviousPhaseItems,
			bLogItemSpawns
		);
	}
}

void UA302ItemSpawnSubsystem::Deinitialize()
{
	if (Orchestrator)
	{
		Orchestrator->Deinitialize();
	}

	Orchestrator = nullptr;
	Super::Deinitialize();
}

void UA302ItemSpawnSubsystem::HandleRoomLevelReady(const FString& RoomCode, EGamePhase CurrentPhase)
{
	if (Orchestrator)
	{
		Orchestrator->HandleRoomLevelReady(RoomCode, CurrentPhase);
	}
}

void UA302ItemSpawnSubsystem::HandleRoomPhaseChanged(const FString& RoomCode, EGamePhase NewPhase)
{
	if (Orchestrator)
	{
		Orchestrator->HandleRoomPhaseChanged(RoomCode, NewPhase);
	}
}

void UA302ItemSpawnSubsystem::ResetRoom(const FString& RoomCode)
{
	if (Orchestrator)
	{
		Orchestrator->ResetRoom(RoomCode);
	}
}
