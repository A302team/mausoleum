// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

UCLASS()
class A302SERVER_API ALobbyGameMode : public AGameMode
{
    GENERATED_BODY()

public:
    ALobbyGameMode();
    virtual void BeginPlay() override;

};
