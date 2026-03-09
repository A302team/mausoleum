// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameMode/A302GameInstance.h" // For FRoomInfo
#include "LobbyGameMode.generated.h"

class ULobbyWidget;
class UWaitingRoomWidget;

UCLASS()
class A302_API ALobbyGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ALobbyGameMode();

    virtual void BeginPlay() override;

    // Lobby properties
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<ULobbyWidget> LobbyWidgetClass;

    UPROPERTY()
    TObjectPtr<ULobbyWidget> LobbyWidget;

    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UWaitingRoomWidget> WaitingRoomWidgetClass;

    UPROPERTY()
    TObjectPtr<UWaitingRoomWidget> WaitingRoomWidget;

    UPROPERTY(BlueprintReadWrite, Category = "Lobby")
    FString CurrentRoomCode;

    UPROPERTY(BlueprintReadWrite, Category = "Lobby")
    FString MyPlayerName;

    UPROPERTY(BlueprintReadWrite, Category = "Lobby")
    bool bIsHost = false;

    // Delegates
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoomCreatedLocal, const FString&, RoomCode);
    UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
    FOnRoomCreatedLocal OnRoomCreated;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRoomJoinedLocal);
    UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
    FOnRoomJoinedLocal OnRoomJoined;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoomListReceivedLocal, const TArray<FRoomInfo>&, RoomList);
    UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
    FOnRoomListReceivedLocal OnRoomListReceived;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerEnteredLocal, const FString&, PlayerName);
    UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
    FOnPlayerEnteredLocal OnPlayerEntered;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerReadyLocal, const FString&, PlayerName);
    UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
    FOnPlayerReadyLocal OnPlayerReady;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameStartedLocal);
    UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
    FOnGameStartedLocal OnGameStarted;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNicknameAvailableLocal);
    UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
    FOnNicknameAvailableLocal OnNicknameAvailable;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerLeftLocal, const FString&, PlayerName);
    UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
    FOnPlayerLeftLocal OnPlayerLeft;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatMessageReceivedLocal, const FString&, PlayerName, const FString&, Message);
    UPROPERTY(BlueprintAssignable, Category = "Lobby|Events")
    FOnChatMessageReceivedLocal OnChatMessageReceived;

    UFUNCTION(BlueprintCallable, Category = "Lobby")
    void SendToServer(const FString& Message);

    UFUNCTION()
    void OnMessageReceived(const FString& Message);

    void ShowWaitingRoom(const FString& RoomCode);
};