#include "GameMode/LobbyMessageRouter.h"

#include "Room/RoomScopeRules.h"
#include "A302RuntimeGuards.h"
#include "Dom/JsonObject.h"
#include "GameMode/A302GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Network/LobbyConstants.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	const FString ItemReceivedType = TEXT("item_received");

	bool TryResolveGameServerEndpoint(const TSharedPtr<FJsonObject>& Data, FString& OutHost, int32& OutPort)
	{
		OutHost.Reset();
		OutPort = 0;
		if (!Data.IsValid())
		{
			return false;
		}

		Data->TryGetStringField(LobbyProtocol::KeyServerIP, OutHost);
		if (OutHost.IsEmpty())
		{
			Data->TryGetStringField(BackendProtocol::KeyGameHost, OutHost);
		}
		if (OutHost.IsEmpty())
		{
			Data->TryGetStringField(LobbyProtocol::KeyGameServerAddress, OutHost);
		}

		double PortNumber = 0.0;
		if (Data->TryGetNumberField(LobbyProtocol::KeyServerPort, PortNumber))
		{
			OutPort = static_cast<int32>(PortNumber);
		}
		else if (Data->TryGetNumberField(BackendProtocol::KeyGamePort, PortNumber))
		{
			OutPort = static_cast<int32>(PortNumber);
		}

		return !OutHost.IsEmpty() && OutPort > 0;
	}

	FString BuildGameTravelURL(const FString& Host, int32 Port, const FString& RoomCode)
	{
		FString TravelURL = FString::Printf(TEXT("%s:%d"), *Host, Port);
		if (!RoomCode.IsEmpty())
		{
			TravelURL += FString::Printf(TEXT("?roomCode=%s"), *RoomCode);
		}
		return TravelURL;
	}
}

void ULobbyMessageRouter::Initialize(UA302GameInstance* InGameInstance)
{
	GameInstance = InGameInstance;
	BuildHandlerMap();
}

void ULobbyMessageRouter::BuildHandlerMap()
{
	PacketHandlers.Empty();
	PacketHandlers.Add(LobbyProtocol::ResRoomCreated, &ULobbyMessageRouter::HandleRoomCreated);
	PacketHandlers.Add(LobbyProtocol::ResRoomJoined, &ULobbyMessageRouter::HandleRoomJoined);
	PacketHandlers.Add(LobbyProtocol::ResPlayerEntered, &ULobbyMessageRouter::HandlePlayerEntered);
	PacketHandlers.Add(LobbyProtocol::ResRoomList, &ULobbyMessageRouter::HandleRoomList);
	PacketHandlers.Add(LobbyProtocol::ResPlayerReady, &ULobbyMessageRouter::HandlePlayerReady);
	PacketHandlers.Add(LobbyProtocol::ResPlayerLeft, &ULobbyMessageRouter::HandlePlayerLeft);
	PacketHandlers.Add(LobbyProtocol::ResGameStarted, &ULobbyMessageRouter::HandleGameStarted);
	PacketHandlers.Add(LobbyProtocol::ResNicknameAvailable, &ULobbyMessageRouter::HandleNicknameAvailable);
	PacketHandlers.Add(LobbyProtocol::ResChatMessage, &ULobbyMessageRouter::HandleChatMessage);
	PacketHandlers.Add(ItemReceivedType, &ULobbyMessageRouter::HandleItemReceived);
	PacketHandlers.Add(LobbyProtocol::ResError, &ULobbyMessageRouter::HandleError);
}

bool ULobbyMessageRouter::TryGetDataObject(const TSharedPtr<FJsonObject>& JsonObject, TSharedPtr<FJsonObject>& OutData) const
{
	OutData.Reset();
	if (!JsonObject.IsValid())
	{
		return false;
	}

	const TSharedPtr<FJsonObject>* DataObject = nullptr;
	if (!JsonObject->TryGetObjectField(*LobbyProtocol::KeyData, DataObject) || DataObject == nullptr)
	{
		return false;
	}

	OutData = *DataObject;
	return OutData.IsValid();
}

bool ULobbyMessageRouter::ShouldProcessRoomScopedMessage(const TSharedPtr<FJsonObject>& Data, const FString& MessageType, FString* OutRoomCode) const
{
	if (!GameInstance)
	{
		return false;
	}

	FString MessageRoomCode;
	const bool bMatchesRoom = A302RoomScope::IsMessageForRoom(GameInstance->CurrentRoomCode, Data, &MessageRoomCode);
	if (OutRoomCode)
	{
		*OutRoomCode = MessageRoomCode;
	}

	if (bMatchesRoom)
	{
		return true;
	}

	UE_LOG(
		LogTemp,
		Verbose,
		TEXT("[LobbyMessageRouter] Ignore %s for another room. current=%s message=%s"),
		MessageType.IsEmpty() ? TEXT("room_scoped_message") : *MessageType,
		*GameInstance->CurrentRoomCode,
		*MessageRoomCode
	);
	return false;
}

void ULobbyMessageRouter::HandleMessage(const FString& Message)
{
	if (!GameInstance)
	{
		return;
	}

	if (A302RuntimeGuards::IsDedicatedServerWorld(GameInstance->GetWorld()))
	{
		return;
	}

	TSharedPtr<FJsonObject> JsonObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);
	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		return;
	}

	FString Type;
	if (!JsonObject->TryGetStringField(LobbyProtocol::KeyType, Type))
	{
		return;
	}

	TSharedPtr<FJsonObject> Data;
	if (!TryGetDataObject(JsonObject, Data))
	{
		return;
	}

	if (const FLobbyPacketHandler* Handler = PacketHandlers.Find(Type))
	{
		(this->*(*Handler))(Data);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[LobbyMessageRouter] Unknown packet type: %s"), *Type);
}

void ULobbyMessageRouter::HandleRoomCreated(const TSharedPtr<FJsonObject>& Data)
{
	if (!GameInstance || !Data.IsValid())
	{
		return;
	}

	GameInstance->CurrentRoomCode = Data->GetStringField(LobbyProtocol::KeyRoomCode);
	GameInstance->MyPlayerName = Data->GetStringField(LobbyProtocol::KeyPlayerName);
	GameInstance->bIsHost = true;
	GameInstance->OnRoomCreated.Broadcast(GameInstance->CurrentRoomCode);
	GameInstance->ShowWaitingRoom(GameInstance->CurrentRoomCode);
}

void ULobbyMessageRouter::HandleRoomJoined(const TSharedPtr<FJsonObject>& Data)
{
	if (!GameInstance || !Data.IsValid())
	{
		return;
	}

	GameInstance->CurrentRoomCode = Data->GetStringField(LobbyProtocol::KeyRoomCode);
	GameInstance->MyPlayerName = Data->GetStringField(LobbyProtocol::KeyPlayerName);
	GameInstance->bIsHost = Data->GetBoolField(LobbyProtocol::KeyIsHost);
	GameInstance->OnRoomJoined.Broadcast();
	GameInstance->ShowWaitingRoom(GameInstance->CurrentRoomCode);
}

void ULobbyMessageRouter::HandlePlayerEntered(const TSharedPtr<FJsonObject>& Data)
{
	if (GameInstance != nullptr && ShouldProcessRoomScopedMessage(Data, LobbyProtocol::ResPlayerEntered))
	{
		GameInstance->OnPlayerEntered.Broadcast(Data->GetStringField(LobbyProtocol::KeyPlayerName));
	}
}

void ULobbyMessageRouter::HandleRoomList(const TSharedPtr<FJsonObject>& Data)
{
	if (!GameInstance || !Data.IsValid())
	{
		return;
	}

	TArray<FRoomInfo> RoomList;
	const TArray<TSharedPtr<FJsonValue>>* Rooms = nullptr;
	if (Data->TryGetArrayField(LobbyProtocol::KeyRooms, Rooms))
	{
		for (const TSharedPtr<FJsonValue>& Room : *Rooms)
		{
			if (!Room.IsValid() || !Room->AsObject().IsValid())
			{
				continue;
			}

			FRoomInfo Info;
			Info.RoomCode = Room->AsObject()->GetStringField(LobbyProtocol::KeyRoomCode);
			Info.PlayerCount = Room->AsObject()->GetIntegerField(LobbyProtocol::KeyPlayerCount);
			RoomList.Add(Info);
		}
	}

	GameInstance->OnRoomListReceived.Broadcast(RoomList);
}

void ULobbyMessageRouter::HandlePlayerReady(const TSharedPtr<FJsonObject>& Data)
{
	if (GameInstance != nullptr && ShouldProcessRoomScopedMessage(Data, LobbyProtocol::ResPlayerReady))
	{
		GameInstance->OnPlayerReady.Broadcast(Data->GetStringField(LobbyProtocol::KeyPlayerName));
	}
}

void ULobbyMessageRouter::HandlePlayerLeft(const TSharedPtr<FJsonObject>& Data)
{
	if (GameInstance != nullptr && ShouldProcessRoomScopedMessage(Data, LobbyProtocol::ResPlayerLeft))
	{
		GameInstance->OnPlayerLeft.Broadcast(Data->GetStringField(LobbyProtocol::KeyPlayerName));
	}
}

void ULobbyMessageRouter::HandleGameStarted(const TSharedPtr<FJsonObject>& Data)
{
	if (!GameInstance || !Data.IsValid())
	{
		return;
	}

	if (GameInstance->CurrentRoomCode.IsEmpty())
	{
		return;
	}

	if (!ShouldProcessRoomScopedMessage(Data, LobbyProtocol::ResGameStarted))
	{
		return;
	}

	GameInstance->OnGameStarted.Broadcast();

	// 룸 대기실(testLevel)에서만 DS 인게임으로 이동한다.
	if (!A302RuntimeGuards::IsLobbyWorld(GameInstance->GetWorld()))
	{
		return;
	}

	FString GameHost;
	int32 GamePort = 0;
	if (!TryResolveGameServerEndpoint(Data, GameHost, GamePort))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[LobbyMessageRouter] game_started received but endpoint is missing. room=%s"),
			*GameInstance->CurrentRoomCode
		);
		return;
	}

	UWorld* World = GameInstance->GetWorld();
	APlayerController* LocalPC = World ? World->GetFirstPlayerController() : nullptr;
	if (!LocalPC)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[LobbyMessageRouter] game_started received but local player controller is null. room=%s"),
			*GameInstance->CurrentRoomCode
		);
		return;
	}

	const FString TravelURL = BuildGameTravelURL(GameHost, GamePort, GameInstance->CurrentRoomCode);
	UE_LOG(
		LogTemp,
		Log,
		TEXT("[LobbyMessageRouter] ClientTravel to DS. room=%s url=%s"),
		*GameInstance->CurrentRoomCode,
		*TravelURL
	);
	LocalPC->ClientTravel(TravelURL, TRAVEL_Absolute);
}

void ULobbyMessageRouter::HandleNicknameAvailable(const TSharedPtr<FJsonObject>& Data)
{
	if (GameInstance)
	{
		GameInstance->OnNicknameAvailable.Broadcast();
	}
}

void ULobbyMessageRouter::HandleChatMessage(const TSharedPtr<FJsonObject>& Data)
{
	if (!GameInstance || !Data.IsValid())
	{
		return;
	}

	if (!ShouldProcessRoomScopedMessage(Data, LobbyProtocol::ResChatMessage))
	{
		return;
	}

	const FString PlayerName = Data->GetStringField(LobbyProtocol::KeyPlayerName);
	const FString ChatMessage = Data->GetStringField(LobbyProtocol::KeyMessage);
	GameInstance->OnChatMessageReceived.Broadcast(PlayerName, ChatMessage);
}

void ULobbyMessageRouter::HandleItemReceived(const TSharedPtr<FJsonObject>& Data)
{
	if (!GameInstance || !Data.IsValid())
	{
		return;
	}

	if (!ShouldProcessRoomScopedMessage(Data, ItemReceivedType))
	{
		return;
	}

	const FString PlayerName = Data->GetStringField(LobbyProtocol::KeyPlayerName);
	const FString ItemType = Data->GetStringField(LobbyProtocol::KeyItemType);
	GameInstance->OnItemReceived.Broadcast(PlayerName, ItemType);
}

void ULobbyMessageRouter::HandleError(const TSharedPtr<FJsonObject>& Data)
{
	if (!Data.IsValid())
	{
		return;
	}

	const FString ErrorMsg = Data->GetStringField(LobbyProtocol::KeyMessage);
	UE_LOG(LogTemp, Warning, TEXT("[LobbyMessageRouter] Error: %s"), *ErrorMsg);
}

