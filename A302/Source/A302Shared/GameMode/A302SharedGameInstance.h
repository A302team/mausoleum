#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "A302SharedGameInstance.generated.h"

UCLASS()
class A302SHARED_API UA302SharedGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadWrite)
	FString CurrentRoomCode;

	UPROPERTY(BlueprintReadWrite)
	FString MyPlayerName;

	UPROPERTY(BlueprintReadWrite)
	bool bIsHost = false;
};
