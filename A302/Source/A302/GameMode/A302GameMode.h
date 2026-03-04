// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "A302GameMode.generated.h"

UCLASS()
class A302_API AA302GameMode : public AGameMode
{
    GENERATED_BODY()

public:
    AA302GameMode();

    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    TObjectPtr<class ASpawnManager> SpawnManager;

    int CurrentStage = 1;

    void SpawnPlayer(APlayerController* PlayerController);
};