// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "A302GameState.generated.h"

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    Waiting,
    InGame,
    End
};

UCLASS()
class A302_API AA302GameState : public AGameState
{
    GENERATED_BODY()

public:
    AA302GameState();

    UPROPERTY(Replicated, BlueprintReadOnly)
    EGamePhase GamePhase = EGamePhase::Waiting;

    UPROPERTY(Replicated, BlueprintReadOnly)
    int32 AlivePlayerCount = 0;

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};