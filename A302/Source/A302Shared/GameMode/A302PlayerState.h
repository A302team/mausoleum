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
class A302SHARED_API AA302PlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    AA302PlayerState();

    UPROPERTY(Replicated, BlueprintReadOnly)
    EPlayerRole PlayerRole = EPlayerRole::Innocent;

    UPROPERTY(ReplicatedUsing = OnRep_IsAlive, BlueprintReadOnly)
    bool bIsAlive = true;

    UPROPERTY(Replicated, BlueprintReadOnly)
    bool bIsEscaped = false;

    UPROPERTY(Replicated, BlueprintReadOnly)
    bool bHasKillItem = false;

    UPROPERTY(ReplicatedUsing = OnRep_RoomCode, BlueprintReadOnly, Category = "Room")
    FString RoomCode;

    UFUNCTION(BlueprintCallable, Category = "Room")
    void SetRoomCode(const FString& InRoomCode);

    UFUNCTION(BlueprintPure, Category = "Room")
    const FString& GetRoomCode() const { return RoomCode; }

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Room")
    bool bGameplayEnabled = false;

    UFUNCTION(BlueprintCallable, Category = "Room")
    void SetGameplayEnabled(bool bInGameplayEnabled);

    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UFUNCTION()
    void OnRep_IsAlive();

    UFUNCTION()
    void OnRep_RoomCode();
};
