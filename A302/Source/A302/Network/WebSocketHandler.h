#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Network/ConnectionHandler.h"
#include "IWebSocket.h"
#include "WebSocketHandler.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWebSocketMessageReceived, const FString&, Message);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnWebSocketBinaryMessageReceived, const TArray<uint8>&);

UCLASS()
class A302_API UWebSocketHandler : public UObject, public IConnectionHandler
{
	GENERATED_BODY()

public:
	virtual void Connect(const FString& URL) override;
	virtual void Disconnect() override;
	virtual void SendMessage(const FString& Message) override;
    virtual void SendBinaryMessage(const TArray<uint8>& MessageData) override;
	virtual bool IsConnected() const override;

	UPROPERTY(BlueprintAssignable, Category = "Network|WebSocket")
	FOnWebSocketMessageReceived OnMessageReceived;

	// C++ 델리게이트를 통해 바이너리 전달
    FOnWebSocketBinaryMessageReceived OnBinaryMessageReceived;

private:
	TSharedPtr<IWebSocket> WebSocket;
};
