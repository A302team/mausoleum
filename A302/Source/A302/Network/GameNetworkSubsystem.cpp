#include "Network/GameNetworkSubsystem.h"
#include "Network/WebSocketHandler.h"
#include "Network/UDPHandler.h"

void UGameNetworkSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Config 로드 명시적 호출 (DefaultEngine.ini의 [/Script/A302.GameNetworkSubsystem] 섹션)
	LoadConfig();

	UE_LOG(LogTemp, Log, TEXT("[UGameNetworkSubsystem] Initialized. ServerIP: %s, Lobby: %d, Voice: %d"), 
		*ServerIP, LobbyPort, VoicePort);

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
	// 외부(블루프린트, GameInstance 등)에서 넘긴 URL을 무시하고,
	// 무조건 Config(ServerIP, Port) 조합의 정제된 자체 URL을 사용하도록 강제합니다.
	if (Protocol == EProtocolType::WebSocket && WebSocketHandler)
	{
		FString FinalURL = GetURL(Protocol);
		UE_LOG(LogTemp, Log, TEXT("[UGameNetworkSubsystem] Connecting WebSocket to: %s"), *FinalURL);
		WebSocketHandler->Connect(FinalURL);
	}
	else if (Protocol == EProtocolType::UDP && UDPHandler)
	{
		FString FinalURL = GetURL(Protocol);
		UE_LOG(LogTemp, Log, TEXT("[UGameNetworkSubsystem] Connecting UDP to: %s"), *FinalURL);
		UDPHandler->Connect(FinalURL);
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
