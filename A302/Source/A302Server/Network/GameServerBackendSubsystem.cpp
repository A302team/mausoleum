#include "Network/GameServerBackendSubsystem.h"
#include "Dom/JsonObject.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Network/LobbyConstants.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

bool UGameServerBackendSubsystem::ShouldOperateAsDedicatedServer() const
{
	if (IsRunningDedicatedServer())
	{
		return true;
	}

	const UGameInstance* GI = GetGameInstance();
	const UWorld* World = GI ? GI->GetWorld() : nullptr;
	return World && World->GetNetMode() == NM_DedicatedServer;
}

void UGameServerBackendSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[UGameServerBackendSubsystem] Initialized. BackendURL=%s AutoConnect=%s"),
		*GetBackendURL(),
		bAutoConnectOnServerStart ? TEXT("true") : TEXT("false")
	);

	if (!ShouldOperateAsDedicatedServer())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[UGameServerBackendSubsystem] Non-dedicated runtime. Skip backend dedicated registration flow."));
		return;
	}

	if (bAutoConnectOnServerStart)
	{
		UE_LOG(LogTemp, Log, TEXT("[UGameServerBackendSubsystem] Dedicated runtime detected. Auto-connecting to backend..."));
		ConnectToBackend();
	}
}

void UGameServerBackendSubsystem::Deinitialize()
{
	DisconnectFromBackend();
	Super::Deinitialize();
}

FString UGameServerBackendSubsystem::GetBackendURL() const
{
	return FA302NetworkEndpointConfig::GetBackendWebSocketURL();
}

void UGameServerBackendSubsystem::ConnectToBackend()
{
	if (!ShouldOperateAsDedicatedServer())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[UGameServerBackendSubsystem] ConnectToBackend ignored in non-dedicated runtime."));
		return;
	}

	if (!SharedClient)
	{
		SharedClient = NewObject<UA302WebSocketClient>(this);
		SharedClient->OnConnected.AddDynamic(this, &UGameServerBackendSubsystem::HandleBackendConnected);
		SharedClient->OnMessageReceived.AddDynamic(this, &UGameServerBackendSubsystem::HandleBackendMessage);
	}
	else if (SharedClient->IsConnected())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[UGameServerBackendSubsystem] Already connected. Skipping duplicate connect."));
		return;
	}

	SharedClient->Connect(GetBackendURL(), TEXT("[UGameServerBackendSubsystem]"));
}

void UGameServerBackendSubsystem::DisconnectFromBackend()
{
	if (SharedClient)
	{
		SharedClient->Disconnect();
		bDedicatedRegistrationSent = false;
	}
}

bool UGameServerBackendSubsystem::IsConnected() const
{
	return SharedClient && SharedClient->IsConnected();
}

void UGameServerBackendSubsystem::SendPacket(const FString& Payload)
{
	if (!SharedClient || !SharedClient->IsConnected())
	{
		UE_LOG(LogTemp, Warning, TEXT("[UGameServerBackendSubsystem] Cannot send packet. Backend not connected."));
		return;
	}

	SharedClient->SendMessage(Payload);
}

void UGameServerBackendSubsystem::HandleBackendConnected()
{
	if (!ShouldOperateAsDedicatedServer())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[UGameServerBackendSubsystem] Backend connected in non-dedicated runtime. Skip dedicated registration."));
		return;
	}

	bDedicatedRegistrationSent = false;
	UE_LOG(LogTemp, Log, TEXT("[UGameServerBackendSubsystem] Connected to backend."));
	RegisterDedicatedIdentity();
}

void UGameServerBackendSubsystem::HandleBackendMessage(const FString& Message)
{
	OnPacketReceived.Broadcast(Message);
}

void UGameServerBackendSubsystem::RegisterDedicatedIdentity()
{
	if (!ShouldOperateAsDedicatedServer())
	{
		return;
	}

	if (bDedicatedRegistrationSent || !SharedClient || !SharedClient->IsConnected())
	{
		return;
	}

	const TSharedPtr<FJsonObject> DataObject = MakeShared<FJsonObject>();
	DataObject->SetStringField(BackendProtocol::KeyServerId, DedicatedServerId);
	DataObject->SetStringField(BackendProtocol::KeyGameHost, FA302NetworkEndpointConfig::GetGameServerHost());
	DataObject->SetNumberField(BackendProtocol::KeyGamePort, FA302NetworkEndpointConfig::GameServerPort);
	DataObject->SetStringField(BackendProtocol::KeyRole, BackendProtocol::RoleDedicated);

	const TSharedPtr<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetStringField(LobbyProtocol::KeyDomain, BackendProtocol::Domain);
	RootObject->SetStringField(LobbyProtocol::KeyType, BackendProtocol::ReqRegisterDedicated);
	RootObject->SetObjectField(LobbyProtocol::KeyData, DataObject);

	FString Payload;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
	if (!FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer))
	{
		return;
	}

	SharedClient->SendMessage(Payload);
	bDedicatedRegistrationSent = true;

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[UGameServerBackendSubsystem] Sent dedicated registration. serverId=%s port=%d"),
		*DedicatedServerId,
		FA302NetworkEndpointConfig::GameServerPort
	);
}
