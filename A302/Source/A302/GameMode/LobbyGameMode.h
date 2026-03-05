// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Network/WebSocketManager.h"
#include "LobbyGameMode.generated.h"

USTRUCT(BlueprintType)
struct FRoomInfo
{
	GENERATED_BODY()

	UPROPERTY()
	FString RoomCode;

	UPROPERTY()
	int32 PlayerCount = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerEntered, const FString &, PlayerName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerReady, const FString &, PlayerName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameStarted);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoomCreated, const FString &, RoomCode);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoomListReceived, const TArray<FRoomInfo> &, RoomList);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRoomJoined);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNicknameAvailable);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerLeft, const FString &, PlayerName);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatMessageReceived, const FString &, PlayerName, const FString &, Message);

UCLASS()
class A302_API ALobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ALobbyGameMode();

	UPROPERTY()
	TObjectPtr<UWebSocketManager> WebSocketManager;

	virtual void BeginPlay() override;

	// Send Message to Server
	UFUNCTION(BlueprintCallable)
	void SendToServer(const FString &Message);

	// Received Server Message
	UFUNCTION()
	void OnMessageReceived(const FString &Message);

	UFUNCTION()
	void ShowWaitingRoom(const FString &RoomCode);

	// Widget Class
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class ULobbyWidget> LobbyWidgetClass;

	UPROPERTY()
	TObjectPtr<class ULobbyWidget> LobbyWidget;

	// Delegate For Widget
	UPROPERTY(BlueprintAssignable)
	FOnPlayerEntered OnPlayerEntered;

	UPROPERTY(BlueprintAssignable)
	FOnPlayerReady OnPlayerReady;

	UPROPERTY(BlueprintAssignable)
	FOnGameStarted OnGameStarted;

	// Room 관련 델리게이드 4개
	UPROPERTY(BlueprintAssignable)
	FOnRoomCreated OnRoomCreated;

	UPROPERTY(BlueprintAssignable)
	FOnRoomListReceived OnRoomListReceived;

	UPROPERTY(BlueprintAssignable)
	FOnRoomJoined OnRoomJoined;

	UPROPERTY(BlueprintAssignable)
	FOnPlayerLeft OnPlayerLeft;

	UPROPERTY(BlueprintAssignable)
	FOnChatMessageReceived OnChatMessageReceived;

	UPROPERTY(BlueprintReadOnly)
	FString CurrentRoomCode;

	UPROPERTY(BlueprintReadOnly)
	FString MyPlayerName;

	UPROPERTY(BlueprintReadOnly)
	bool bIsHost = false;

	// 닉네임 체크
	UPROPERTY(BlueprintAssignable)
	FOnNicknameAvailable OnNicknameAvailable;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UWaitingRoomWidget> WaitingRoomWidgetClass;

	UPROPERTY()
	TObjectPtr<class UWaitingRoomWidget> WaitingRoomWidget;
};
