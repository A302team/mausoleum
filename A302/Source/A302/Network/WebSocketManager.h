// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "IWebSocket.h"
#include "WebSocketManager.generated.h"

/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMessageReceived, const FString&, Message);

UCLASS()
class A302_API UWebSocketManager : public UObject
{
	GENERATED_BODY()
	

public:
	// Connect To Server
	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void Connect(const FString& URL);

	// Send Message To Server
	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void SendMessage(const FString& Message);

	// Disconnect From Server
	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	void Disconnect();

	// Check If Connected
	UFUNCTION(BlueprintCallable, Category = "WebSocket")
	bool IsConnected() const;

	// Message Received Delegate
	UPROPERTY(BlueprintAssignable, Category = "WebSocket")
	FOnMessageReceived OnMessageReceived;

private:
	TSharedPtr<IWebSocket> WebSocket;

};
