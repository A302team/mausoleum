#include "Network/A302ServerBackendRouter.h"
#include "GameMode/A302GameMode.h"
#include "Network/LobbyConstants.h"
#include "Network/GameServerBackendSubsystem.h"
#include "Network/A302NetworkEndpointConfig.h"
#include "Room/RoomScopeRules.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Room/A302RoomRuntimeSubsystem.h"
#include "Room/RoomMembershipRegistry.h"

void UA302ServerBackendRouter::Initialize(AA302GameMode* InGameMode)
{
	OwningGameMode = InGameMode;
}

void UA302ServerBackendRouter::HandleMessage(const FString& Message)
{
	if (!OwningGameMode.IsValid()) return;

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject))
		return;

	FString Type;
	if (!JsonObject->TryGetStringField(LobbyProtocol::KeyType, Type))
	{
		return;
	}

	const TSharedPtr<FJsonObject>* DataPtr = nullptr;
	if (!JsonObject->TryGetObjectField(LobbyProtocol::KeyData, DataPtr) || DataPtr == nullptr || !DataPtr->IsValid())
	{
		return;
	}
	const TSharedPtr<FJsonObject> Data = *DataPtr;

	FString Domain;
	JsonObject->TryGetStringField(LobbyProtocol::KeyDomain, Domain);
	
	// Backend Domain 처리
	if (Domain == BackendProtocol::Domain)
	{
		if (Type == BackendProtocol::ReqPrepareGame)
		{
			HandlePrepareGame(Data);
			return;
		}
		return;
	}

	// Lobby Domain (Chat 등) 처리
	if (Type == LobbyProtocol::ResChatMessage)
	{
		HandleChatMessage(Data, Message);
	}
}

void UA302ServerBackendRouter::HandlePrepareGame(const TSharedPtr<FJsonObject>& Data)
{
	if (!OwningGameMode.IsValid() || !Data.IsValid()) return;
	
    UGameServerBackendSubsystem* BackendSubsystem = OwningGameMode->GetBackendSubsystem();
    if (!BackendSubsystem) return;

	FString RoomCode;
	if (!Data->TryGetStringField(BackendProtocol::KeyRoomCode, RoomCode) || RoomCode.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[BackendRouter] prepare_game ignored: roomCode missing"));
		return;
	}
	RoomCode = A302RoomScope::NormalizeRoomCode(RoomCode);

	OwningGameMode->StartRoomGameplay(RoomCode);

	const int32 PlayersInRoom = (OwningGameMode->GetRoomMembershipRegistry() && OwningGameMode->GetWorld())
		? OwningGameMode->GetRoomMembershipRegistry()->CountPlayersInRoom(OwningGameMode->GetWorld(), RoomCode)
		: 0;

    const TSharedPtr<FJsonObject> AckData = MakeShared<FJsonObject>();
    AckData->SetStringField(BackendProtocol::KeyRoomCode, RoomCode);
    AckData->SetStringField(BackendProtocol::KeyServerId, BackendSubsystem->DedicatedServerId);
    AckData->SetStringField(BackendProtocol::KeyGameHost, FA302NetworkEndpointConfig::GetGameServerHost());
    AckData->SetNumberField(BackendProtocol::KeyGamePort, FA302NetworkEndpointConfig::GameServerPort);

    const TSharedPtr<FJsonObject> AckMessage = MakeShared<FJsonObject>();
    AckMessage->SetStringField(LobbyProtocol::KeyDomain, BackendProtocol::Domain);
    AckMessage->SetStringField(LobbyProtocol::KeyType, BackendProtocol::ReqDedicatedReady);
    AckMessage->SetObjectField(LobbyProtocol::KeyData, AckData);

    FString Payload;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
    if (!FJsonSerializer::Serialize(AckMessage.ToSharedRef(), Writer))
    {
        UE_LOG(LogTemp, Warning, TEXT("[BackendRouter] Failed to serialize dedicated_ready payload"));
        return;
    }

    BackendSubsystem->SendPacket(Payload);

    UE_LOG(
        LogTemp,
        Log,
		TEXT("[BackendRouter] prepare_game handled. room=%s ack=dedi_ready players=%d"),
		*RoomCode,
        PlayersInRoom
	);
}

void UA302ServerBackendRouter::HandleChatMessage(const TSharedPtr<FJsonObject>& Data, const FString& RawMessage)
{
	if (!OwningGameMode.IsValid()) return;

	const FString MessageRoomCode = A302RoomScope::ExtractRoomCode(Data);
	if (MessageRoomCode.IsEmpty()) return;

	UWorld* World = OwningGameMode->GetWorld();
	URoomMembershipRegistry* Registry = OwningGameMode->GetRoomMembershipRegistry();

	if (!Registry || !World || Registry->CountPlayersInRoom(World, MessageRoomCode) <= 0)
	{
		return;
	}

	// Chat Logging 및 Broadcast (GameMode의 Delegate 호출)
	FString PlayerName = Data->GetStringField(LobbyProtocol::KeyPlayerName);
	FString ChatMessage = Data->GetStringField(LobbyProtocol::KeyMessage);
	
	OwningGameMode->OnInGameChatReceived.Broadcast(PlayerName, ChatMessage);
}

void UA302ServerBackendRouter::NotifyGameFinished(const FString& RoomCode)
{
	if (!OwningGameMode.IsValid()) return;

	UGameServerBackendSubsystem* BackendSubsystem = OwningGameMode->GetBackendSubsystem();
	if (!BackendSubsystem) return;

	const TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(BackendProtocol::KeyRoomCode, RoomCode);
	Data->SetStringField(BackendProtocol::KeyServerId, BackendSubsystem->DedicatedServerId);

	const TSharedPtr<FJsonObject> Message = MakeShared<FJsonObject>();
	Message->SetStringField(LobbyProtocol::KeyDomain, BackendProtocol::Domain);
	Message->SetStringField(LobbyProtocol::KeyType, BackendProtocol::ReqFinishGame);
	Message->SetObjectField(LobbyProtocol::KeyData, Data);

	FString Payload;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
	if (FJsonSerializer::Serialize(Message.ToSharedRef(), Writer))
	{
		BackendSubsystem->SendPacket(Payload);
		UE_LOG(LogTemp, Log, TEXT("[BackendRouter] Sent finish_game for room: %s"), *RoomCode);
	}
}

void UA302ServerBackendRouter::NotifyPlayerLogout(const FString& PlayerName, const FString& RoomCode)
{
	if (!OwningGameMode.IsValid() || PlayerName.IsEmpty()) return;

	UGameServerBackendSubsystem* BackendSubsystem = OwningGameMode->GetBackendSubsystem();
	if (!BackendSubsystem) return;

	const TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(LobbyProtocol::KeyPlayerName, PlayerName);
	Data->SetStringField(LobbyProtocol::KeyRoomCode, RoomCode);
	Data->SetStringField(BackendProtocol::KeyServerId, BackendSubsystem->DedicatedServerId);

	const TSharedPtr<FJsonObject> Message = MakeShared<FJsonObject>();
	Message->SetStringField(LobbyProtocol::KeyDomain, BackendProtocol::Domain);
	Message->SetStringField(LobbyProtocol::KeyType, LobbyProtocol::ReqLeaveRoom); // 서버에서 직접 LeaveRoom 요청을 보내 퇴장 처리 유도
	Message->SetObjectField(LobbyProtocol::KeyData, Data);

	FString Payload;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Payload);
	if (FJsonSerializer::Serialize(Message.ToSharedRef(), Writer))
	{
		BackendSubsystem->SendPacket(Payload);
		UE_LOG(LogTemp, Log, TEXT("[BackendRouter] Sent logout notification for player: %s room: %s"), *PlayerName, *RoomCode);
	}
}
