// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "A302GameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInGameChatReceived, const FString &, PlayerName, const FString &, Message);

class AMyCharacter;

UCLASS()
class A302_API AA302GameMode : public AGameMode
{
    GENERATED_BODY()

public:
    AA302GameMode();

    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<class UChatWidget> ChatWidgetClass;

    UPROPERTY()
    TObjectPtr<class UChatWidget> ChatWidget;

    UPROPERTY(EditDefaultsOnly, Category = "Spawn")
    TSubclassOf<AMyCharacter> CharacterClass;

    UPROPERTY(BlueprintReadWrite)
    FString MyPlayerName;

    UPROPERTY(BlueprintReadWrite)
    FString CurrentRoomCode;

    UPROPERTY(BlueprintReadWrite)
    bool bIsHost;

    UPROPERTY(BlueprintAssignable)
    FOnInGameChatReceived OnInGameChatReceived;

    UFUNCTION()
    void SendToServer(const FString &Message);

    virtual void PostLogin(APlayerController *NewPlayer) override;
    virtual void Logout(AController *Exiting) override;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY()
    bool bServerTravelRequested = false;

    UPROPERTY()
    TObjectPtr<class ASpawnManager> SpawnManager;

    UFUNCTION()
    void OnMessageReceived(const FString &Message);

    int CurrentStage = 1;

    void SpawnPlayer(APlayerController *PlayerController);
};
