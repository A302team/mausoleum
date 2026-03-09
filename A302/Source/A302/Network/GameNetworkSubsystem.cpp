#include "Network/GameNetworkSubsystem.h"
#include "Network/WebSocketHandler.h"
#include "Network/UDPHandler.h"

void UGameNetworkSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("[UGameNetworkSubsystem] Initialized."));

	WebSocketHandler = NewObject<UWebSocketHandler>(this);
	
	if (WebSocketHandler)
	{
		WebSocketHandler->OnMessageReceived.AddDynamic(this, &UGameNetworkSubsystem::HandleWebSocketMessage);
		WebSocketHandler->OnBinaryMessageReceived.AddUObject(this, &UGameNetworkSubsystem::HandleWebSocketBinaryMessage);
	}

	UDPHandler = NewObject<UUDPHandler>(this);

	if (UDPHandler)
	{
		UDPHandler->OnMessageReceived.AddDynamic(this, &UGameNetworkSubsystem::HandleUDPMessage);
		UDPHandler->OnBinaryMessageReceived.AddUObject(this, &UGameNetworkSubsystem::HandleUDPBinaryMessage);
	}
}

void UGameNetworkSubsystem::Deinitialize()
{
	if (WebSocketHandler)
	{
		WebSocketHandler->Disconnect();
		WebSocketHandler = nullptr;
	}

	if (UDPHandler)
	{
		UDPHandler->Disconnect();
		UDPHandler = nullptr;
	}

	Super::Deinitialize();
}

void UGameNetworkSubsystem::Connect(EProtocolType Protocol, const FString& URL)
{
	if (Protocol == EProtocolType::WebSocket && WebSocketHandler)
	{
		WebSocketHandler->Connect(URL);
	}
	else if (Protocol == EProtocolType::UDP && UDPHandler)
	{
		UDPHandler->Connect(URL);
	}
}

void UGameNetworkSubsystem::Disconnect(EProtocolType Protocol)
{
	if (Protocol == EProtocolType::WebSocket && WebSocketHandler)
	{
		WebSocketHandler->Disconnect();
	}
	else if (Protocol == EProtocolType::UDP && UDPHandler)
	{
		UDPHandler->Disconnect();
	}
}

bool UGameNetworkSubsystem::IsConnected(EProtocolType Protocol) const
{
	if (Protocol == EProtocolType::WebSocket && WebSocketHandler)
	{
		return WebSocketHandler->IsConnected();
	}
	else if (Protocol == EProtocolType::UDP && UDPHandler)
	{
		return UDPHandler->IsConnected();
	}
	return false;
}

void UGameNetworkSubsystem::SendPacket(EProtocolType Protocol, const FString& Payload)
{
	if (Protocol == EProtocolType::WebSocket)
	{
        if (WebSocketHandler && WebSocketHandler->IsConnected())
        {
		    WebSocketHandler->SendMessage(Payload);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[UGameNetworkSubsystem] Cannot SendPacket. WebSocket not connected."));
        }
	}
	else if (Protocol == EProtocolType::UDP)
	{
        if (UDPHandler && UDPHandler->IsConnected())
        {
		    UDPHandler->SendMessage(Payload);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[UGameNetworkSubsystem] Cannot SendPacket. UDP not connected."));
        }
	}
}

void UGameNetworkSubsystem::SendBinaryPacket(EProtocolType Protocol, const TArray<uint8>& Payload)
{
	if (Protocol == EProtocolType::WebSocket)
	{
        if (WebSocketHandler && WebSocketHandler->IsConnected())
        {
		    WebSocketHandler->SendBinaryMessage(Payload);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[UGameNetworkSubsystem] Cannot SendBinaryPacket. WebSocket not connected."));
        }
	}
	else if (Protocol == EProtocolType::UDP)
	{
        if (UDPHandler && UDPHandler->IsConnected())
        {
		    UDPHandler->SendBinaryMessage(Payload);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[UGameNetworkSubsystem] Cannot SendBinaryPacket. UDP not connected."));
        }
	}
}

void UGameNetworkSubsystem::HandleWebSocketMessage(const FString& Message)
{
	OnPacketReceived.Broadcast(Message);
}

void UGameNetworkSubsystem::HandleWebSocketBinaryMessage(const TArray<uint8>& MessageData)
{
	OnBinaryPacketReceived.Broadcast(MessageData);
}

void UGameNetworkSubsystem::HandleUDPMessage(const FString& Message)
{
	OnPacketReceived.Broadcast(Message);
}

void UGameNetworkSubsystem::HandleUDPBinaryMessage(const TArray<uint8>& MessageData)
{
	OnBinaryPacketReceived.Broadcast(MessageData);
}
