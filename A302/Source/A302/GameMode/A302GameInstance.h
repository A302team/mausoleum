// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "A302GameInstance.generated.h"

/**
 *
 */
UCLASS()
class A302_API UA302GameInstance : public UGameInstance
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
