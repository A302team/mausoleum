// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "A302PlayerState.generated.h"

UENUM(BlueprintType)
enum class EPlayerRole : uint8
{
    Innocent,  // 선인
    Evil       // 악인
};

UCLASS()
class A302_API AA302PlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    AA302PlayerState();

    UPROPERTY(Replicated, BlueprintReadOnly)
    EPlayerRole PlayerRole = EPlayerRole::Innocent;

    UPROPERTY(Replicated, BlueprintReadOnly)
    bool bIsAlive = true;

    UPROPERTY(Replicated, BlueprintReadOnly)
    bool bIsEscaped = false;

    UPROPERTY(Replicated, BlueprintReadOnly)
    bool bHasKillItem = false;

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};