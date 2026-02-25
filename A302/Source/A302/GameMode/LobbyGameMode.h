// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Network/WebSocketManager.h"
#include "LobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class A302_API ALobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALobbyGameMode();

	UPROPERTY()
	TObjectPtr<UWebSocketManager> WebSocketManager;

	virtual void BeginPlay() override;

	// Send Message to Server
	UFUNCTION(BlueprintCallable)
	void SendToServer(const FString& Message);

	// Received Server Message
	UFUNCTION()
	void OnMessageReceived(const FString& Message);

	// Widget Class
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class ULobbyWidget> LobbyWidgetClass;

	UPROPERTY()
	TObjectPtr<class ULobbyWidget> LobbyWidget;
};
