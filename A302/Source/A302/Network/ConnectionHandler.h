#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ConnectionHandler.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UConnectionHandler : public UInterface
{
	GENERATED_BODY()
};

class A302_API IConnectionHandler
{
	GENERATED_BODY()

public:
	virtual void Connect(const FString& URL) = 0;
	virtual void Disconnect() = 0;
	// text 전송
	virtual void SendMessage(const FString& Message) = 0;
	// binary 전송 (Protobuf 등)
    virtual void SendBinaryMessage(const TArray<uint8>& MessageData) = 0;
	virtual bool IsConnected() const = 0;
};
