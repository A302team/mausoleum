#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "A302WebSocketClient.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnA302WebSocketMessageReceived, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnA302WebSocketConnected);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnA302WebSocketBinaryMessageReceived, const TArray<uint8>&);

UCLASS()
class A302SHARED_API UA302WebSocketClient : public UObject
{
	GENERATED_BODY()

public:
	void Connect(const FString& URL, const FString& LogPrefix);
	void Disconnect();
	void SendMessage(const FString& Message);
	void SendBinaryMessage(const TArray<uint8>& MessageData);
	bool IsConnected() const;

	UPROPERTY(BlueprintAssignable, Category = "Network|WebSocket")
	FOnA302WebSocketMessageReceived OnMessageReceived;

	UPROPERTY(BlueprintAssignable, Category = "Network|WebSocket")
	FOnA302WebSocketConnected OnConnected;

	FOnA302WebSocketBinaryMessageReceived OnBinaryMessageReceived;

private:
	TSharedPtr<class IWebSocket> WebSocket;
	FString CurrentLogPrefix;
};
