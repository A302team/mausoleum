#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace A302RuntimeGuards
{
	inline bool MapNameContains(const UWorld* World, const TCHAR* Fragment)
	{
		if (!World || !Fragment)
		{
			return false;
		}

		return World->GetMapName().Contains(Fragment);
	}

	inline bool IsLobbyWorld(const UWorld* World)
	{
		return MapNameContains(World, TEXT("testLevel"));
	}

	inline bool IsInGameWorld(const UWorld* World)
	{
		return MapNameContains(World, TEXT("MyTestLevel")) || MapNameContains(World, TEXT("MyMap"));
	}

	inline bool IsInGameWorld(const UObject* WorldContextObject)
	{
		return WorldContextObject ? IsInGameWorld(WorldContextObject->GetWorld()) : false;
	}

	inline bool IsDedicatedServerWorld(const UObject* WorldContextObject)
	{
		if (!WorldContextObject)
		{
			return false;
		}

		const UWorld* World = WorldContextObject->GetWorld();
		return World && World->GetNetMode() == NM_DedicatedServer;
	}

	inline bool ShouldRunClientOnlyLogic(const UObject* WorldContextObject)
	{
		return !IsDedicatedServerWorld(WorldContextObject);
	}

	inline bool ShouldInitializeLocalControllerUI(const APlayerController* PlayerController)
	{
		return PlayerController && PlayerController->IsLocalController() && ShouldRunClientOnlyLogic(PlayerController);
	}
}
