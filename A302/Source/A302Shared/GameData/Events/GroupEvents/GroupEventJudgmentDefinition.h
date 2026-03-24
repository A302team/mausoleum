#pragma once

#include "CoreMinimal.h"
#include "GameData/Events/GroupEvents/GroupEventDefinition.h"
#include "GroupEventJudgmentDefinition.generated.h"

USTRUCT(BlueprintType)
struct FGroupEventJudgmentPayload
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GroupEvent|Payload", meta = (ClampMin = "1.0"))
	float VoteDurationSeconds = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GroupEvent|Payload", meta = (ClampMin = "1"))
	int32 MaliceThreshold = 2;
};

UCLASS(BlueprintType)
class A302SHARED_API UGroupEventJudgmentDefinition : public UGroupEventDefinition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GroupEvent")
	FGroupEventJudgmentPayload Payload;
};

