#pragma once

#include "CoreMinimal.h"
#include "GameMode/A302GameState.h"
#include "Object/SpawnArea.h"
#include "ItemSpawnArea.generated.h"

UCLASS()
class A302SHARED_API AItemSpawnArea : public ASpawnArea
{
	GENERATED_BODY()

public:
	// 비어 있으면 모든 페이즈에서 유효합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Spawn")
	TArray<EGamePhase> AllowedPhases;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Spawn", meta = (ClampMin = "0.0"))
	float AreaWeight = 1.0f;

	UFUNCTION(BlueprintPure, Category = "Item Spawn")
	bool IsPhaseAllowed(EGamePhase InPhase) const;
};

