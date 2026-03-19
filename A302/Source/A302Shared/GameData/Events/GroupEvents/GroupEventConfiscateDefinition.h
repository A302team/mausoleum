#pragma once

#include "CoreMinimal.h"
#include "GameData/Events/GroupEvents/GroupEventDefinition.h"
#include "GroupEventConfiscateDefinition.generated.h"

USTRUCT(BlueprintType)
struct FGroupEventConfiscatePayload
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GroupEvent|Payload", meta = (ClampMin = "1.0"))
	float VoteDurationSeconds = 10.0f;
};

UCLASS(BlueprintType)
class A302SHARED_API UGroupEventConfiscateDefinition : public UGroupEventDefinition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GroupEvent")
	FGroupEventConfiscatePayload Payload;
};
