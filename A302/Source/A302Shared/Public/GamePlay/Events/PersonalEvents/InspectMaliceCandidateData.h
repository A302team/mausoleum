#pragma once

#include "CoreMinimal.h"
#include "InspectMaliceCandidateData.generated.h"

USTRUCT(BlueprintType)
struct FInspectMaliceCandidateData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	int32 PlayerId = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite)
	FString PlayerName;

	UPROPERTY(BlueprintReadWrite)
	int32 MaliceCount = 0;
};

