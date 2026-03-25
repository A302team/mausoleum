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
	// 우선순위 1: AllowedPhases에 명시된 페이즈만 유효.
	// 우선순위 2: AllowedPhases가 비어 있으면 TargetStage(1,2,3)를 Phase0,1,2로 매핑.
	// 둘 다 미설정(AllowedPhases 비어있고 TargetStage<=0)이면 스폰 영역으로 사용하지 않습니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Spawn")
	TArray<EGamePhase> AllowedPhases;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Spawn", meta = (ClampMin = "0.0"))
	float AreaWeight = 1.0f;

	UFUNCTION(BlueprintPure, Category = "Item Spawn")
	bool IsPhaseAllowed(EGamePhase InPhase) const;
};
