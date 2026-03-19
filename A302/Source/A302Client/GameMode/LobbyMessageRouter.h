#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LobbyMessageRouter.generated.h"

class FJsonObject;
class UA302GameInstance;

UCLASS()
class A302CLIENT_API ULobbyMessageRouter : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void Initialize(UA302GameInstance* InGameInstance);

	UFUNCTION()
	void HandleMessage(const FString& Message);

private:
	using FLobbyPacketHandler = void (ULobbyMessageRouter::*)(const TSharedPtr<FJsonObject>&);

	void BuildHandlerMap();
	bool TryGetDataObject(const TSharedPtr<FJsonObject>& JsonObject, TSharedPtr<FJsonObject>& OutData) const;
	bool ShouldProcessRoomScopedMessage(const TSharedPtr<FJsonObject>& Data, const FString& MessageType, FString* OutRoomCode = nullptr) const;
	void HandleRoomCreated(const TSharedPtr<FJsonObject>& Data);
	void HandleRoomJoined(const TSharedPtr<FJsonObject>& Data);
	void HandlePlayerEntered(const TSharedPtr<FJsonObject>& Data);
	void HandleRoomList(const TSharedPtr<FJsonObject>& Data);
	void HandlePlayerReady(const TSharedPtr<FJsonObject>& Data);
	void HandlePlayerLeft(const TSharedPtr<FJsonObject>& Data);
	void HandleHostChanged(const TSharedPtr<FJsonObject>& Data);
	void HandleGameStarted(const TSharedPtr<FJsonObject>& Data);
	void HandleNicknameAvailable(const TSharedPtr<FJsonObject>& Data);
	void HandleChatMessage(const TSharedPtr<FJsonObject>& Data);
	void HandleItemReceived(const TSharedPtr<FJsonObject>& Data);
	void HandleError(const TSharedPtr<FJsonObject>& Data);

	UPROPERTY(Transient)
	TObjectPtr<UA302GameInstance> GameInstance = nullptr;

	TMap<FString, FLobbyPacketHandler> PacketHandlers;
};
