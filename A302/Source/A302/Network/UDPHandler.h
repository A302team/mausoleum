#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Network/ConnectionHandler.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Common/UdpSocketReceiver.h"
#include "UDPHandler.generated.h"


class FSocket;
class FUdpSocketReceiver;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUDPMessageReceived, const FString&, Message);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnUDPBinaryMessageReceived, const TArray<uint8>&);

/**
 * 기본 UDP 핸들러 (Unreal Sockets 이용)
 * 송신 + 수신 모두 지원
 */
UCLASS()
class A302_API UUDPHandler : public UObject, public IConnectionHandler
{
	GENERATED_BODY()

public:
	virtual void Connect(const FString& URL) override;
	virtual void Disconnect() override;
	virtual void SendMessage(const FString& Message) override;
	virtual void SendBinaryMessage(const TArray<uint8>& MessageData) override;
	virtual bool IsConnected() const override;

	UPROPERTY(BlueprintAssignable, Category = "Network|UDP")
	FOnUDPMessageReceived OnMessageReceived;

	FOnUDPBinaryMessageReceived OnBinaryMessageReceived;

private:
	bool ParseUrl(const FString& URL, FString& OutHostName, int32& OutPort) const;
	void StartSocket(const FString& URL, int32 Port, const TSharedRef<FInternetAddr>& ResolvedAddr);

	void OnDataReceived(const FArrayReaderPtr& Data, const FIPv4Endpoint& Sender);

	FSocket* UDPSocket = nullptr;
	TSharedPtr<FInternetAddr> TargetAddr;
	FUdpSocketReceiver* SocketReceiver = nullptr;
};
