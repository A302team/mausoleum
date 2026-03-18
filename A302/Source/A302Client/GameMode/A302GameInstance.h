// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameMode/A302SharedGameInstance.h"
#include "A302GameInstance.generated.h"

class FJsonObject;
class ULobbyMessageRouter;
class ULevelStreamingDynamic;

USTRUCT(BlueprintType)
struct FRoomInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString RoomCode;

	UPROPERTY(BlueprintReadWrite)
	int32 PlayerCount = 0;
};

// 플레이어가 방에서 할 수 있는 행동에 대한 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerEntered, const FString &, PlayerName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerReady, const FString &, PlayerName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameStarted);

// 플레이어가 방에서 나갔을 때 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerLeft, const FString &, PlayerName);

// 플레이어 방 만들 때 호출하는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoomCreated, const FString &, RoomCode);

// 방 찾을 때 할 수 있는 행동에 대한 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoomListReceived, const TArray<FRoomInfo> &, RoomList);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRoomJoined);

// 닉네임 체크 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNicknameAvailable);

// 채팅 보내고 받는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatMessageReceived, const FString &, PlayerName, const FString &, Message);

// 아이템 획득 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemReceived, const FString &, PlayerName, const FString &, ItemType);

UCLASS()
class A302CLIENT_API UA302GameInstance : public UA302SharedGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void OnStart() override;

	UFUNCTION()
	void OnMapLoaded(UWorld *LoadedWorld);

	// WebSocket
	UPROPERTY()
	TObjectPtr<class UGameNetworkSubsystem> GameNetworkSubsystem;

	UFUNCTION(BlueprintCallable)
	void ConnectToServer(const FString &URL);

	UFUNCTION(BlueprintCallable)
	void SendToServer(const FString &Message);

	// 델리게이트
	UPROPERTY(BlueprintAssignable)
	FOnRoomCreated OnRoomCreated;

	UPROPERTY(BlueprintAssignable)
	FOnRoomJoined OnRoomJoined;

	UPROPERTY(BlueprintAssignable)
	FOnRoomListReceived OnRoomListReceived;

	UPROPERTY(BlueprintAssignable)
	FOnPlayerEntered OnPlayerEntered;

	UPROPERTY(BlueprintAssignable)
	FOnPlayerReady OnPlayerReady;

	UPROPERTY(BlueprintAssignable)
	FOnPlayerLeft OnPlayerLeft;

	UPROPERTY(BlueprintAssignable)
	FOnGameStarted OnGameStarted;

	UPROPERTY(BlueprintAssignable)
	FOnNicknameAvailable OnNicknameAvailable;

	UPROPERTY(BlueprintAssignable)
	FOnChatMessageReceived OnChatMessageReceived;

	UPROPERTY(BlueprintAssignable)
	FOnItemReceived OnItemReceived;

	// 위젯 클래스
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class ULobbyWidget> LobbyWidgetClass;

	UPROPERTY()
	TObjectPtr<class ULobbyWidget> LobbyWidget;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UWaitingRoomWidget> WaitingRoomWidgetClass;

	UPROPERTY()
	TObjectPtr<class UWaitingRoomWidget> WaitingRoomWidget;

	// 위젯 표시
	void ShowWaitingRoom(const FString &RoomCode);

private:
	UFUNCTION()
	void OnMessageReceived(const FString &Message);

	bool TryCreateLobbyWidget(UWorld *LoadedWorld);
	void EnsureLocalRoomLevelInstance(UWorld* LoadedWorld);
	FString ResolveRoomCodeForInGameWorld(UWorld* LoadedWorld) const;

	UPROPERTY(Transient)
	TObjectPtr<ULobbyMessageRouter> MessageRouter;

	UPROPERTY(Transient)
	TObjectPtr<ULevelStreamingDynamic> LocalRoomStreamingLevel;

	UPROPERTY(Transient)
	FString LocalRoomStreamingRoomCode;

	FTimerHandle LobbyWidgetRetryTimerHandle;

	UFUNCTION()
	void OnWorldAdded(UWorld *World);
};
