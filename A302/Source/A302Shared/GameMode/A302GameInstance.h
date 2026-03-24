// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "A302GameInstance.generated.h"

class FJsonObject;
class ULevelStreamingDynamic;
class UButton;
class UEditableTextBox;
class UUserWidget;
class UWorld;

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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerLeft, const FString &, PlayerName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoomCreated, const FString &, RoomCode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoomListReceived, const TArray<FRoomInfo> &, RoomList);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRoomJoined);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnNicknameAvailable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnChatMessageReceived, const FString &, PlayerName, const FString &, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemReceived, const FString &, PlayerName, const FString &, ItemType);

/**
 * A302GameInstance
 * Dedicated Server와 Client 모두에서 사용되는 기본 게임 인스턴스입니다.
 * 클라이언트 전용 로직(UI, WebSocket)은 헤더 의존성을 피하기 위해 최소화되었습니다.
 */
UCLASS()
class A302SHARED_API UA302GameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UA302GameInstance();
	virtual void Init() override;
	virtual void OnStart() override;

	// Shared State
	UPROPERTY(BlueprintReadWrite, Category = "Game")
	FString CurrentRoomCode;

	UPROPERTY(BlueprintReadWrite, Category = "Game")
	FString MyPlayerName;

	UPROPERTY(BlueprintReadWrite, Category = "Game")
	bool bIsHost = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Config")
	int32 MaxNicknameLength = 20;

	UFUNCTION(BlueprintPure, Category = "Game")
	bool IsNicknameValid(const FString& Nickname, FString& OutErrorMsg) const;

	// Client-side Network (Generic pointers to avoid A302Client dependency)
	UPROPERTY()
	TObjectPtr<UObject> GameNetworkSubsystem;

	UPROPERTY()
	TObjectPtr<UObject> MessageRouter;

	UFUNCTION(BlueprintCallable, Category = "Network")
	void ConnectToServer(const FString &URL);

	UFUNCTION(BlueprintCallable, Category = "Network")
	void SendToServer(const FString &Message);

	UFUNCTION(BlueprintCallable, Category = "Loading")
	void BeginStartGameLoadingTransition(float TimeoutSeconds = 5.0f);

	UFUNCTION(BlueprintCallable, Category = "Loading")
	void CancelStartGameLoadingTransition();

	void NotifyLocalGameplayPawnReady();

	UFUNCTION(BlueprintPure, Category = "Loading")
	bool IsWaitingForRoomStreamingReady() const { return bWaitingForRoomStreamingReady; }

	UFUNCTION(BlueprintPure, Category = "Loading")
	bool CanInitializeGameplayUI() const;

	UFUNCTION(BlueprintPure, Category = "Loading")
	float GetLoadingProgress() const { return LoadingProgress; }

	// Delegates
	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnRoomCreated OnRoomCreated;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnRoomJoined OnRoomJoined;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnRoomListReceived OnRoomListReceived;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPlayerEntered OnPlayerEntered;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPlayerReady OnPlayerReady;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnPlayerLeft OnPlayerLeft;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnGameStarted OnGameStarted;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnNicknameAvailable OnNicknameAvailable;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnChatMessageReceived OnChatMessageReceived;

	UPROPERTY(BlueprintAssignable, Category = "Events")
	FOnItemReceived OnItemReceived;

	// UI Widgets (Soft references to avoid loading on Dedicated Server)
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSoftClassPtr<UUserWidget> LobbyWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> LobbyWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSoftClassPtr<UUserWidget> WaitingRoomWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> WaitingRoomWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSoftClassPtr<UUserWidget> LoadingWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> LoadingWidget;

	UFUNCTION(BlueprintCallable, Category = "UI")
	void ShowWaitingRoom(const FString &RoomCode);

	UFUNCTION()
	void OnMapLoaded(UWorld *LoadedWorld);

	UFUNCTION()
	void HandleFallbackLobbyCreateRoomClicked();

	UFUNCTION()
	void HandleFallbackLobbyEnterRoomClicked();

	UFUNCTION()
	void HandleFallbackLobbyFindRoomClicked();

	UFUNCTION()
	void HandleFallbackLobbyExitClicked();

	UFUNCTION()
	void HandleFallbackLobbyRoomCreated(const FString& RoomCode);

	UFUNCTION()
	void HandleFallbackLobbyNicknameAvailable();

private:
	UFUNCTION()
	void OnMessageReceived(const FString &Message);

	void SetupFallbackLobbyBindings();
	UButton* FindLobbyButton(FName WidgetName) const;
	UEditableTextBox* FindLobbyEditableTextBox(FName WidgetName) const;
	void SendLobbyNicknameCheck(const FString& PlayerName);
	void SendCreateRoomRequest(const FString& PlayerName);
	void SendJoinRoomRequest(const FString& RoomCode, const FString& PlayerName);
	void OpenRoomListPopupFallback();
	bool ValidateLobbyPlayerName(FString& OutPlayerName) const;
	bool TryCreateLobbyWidget(UWorld *LoadedWorld);
	bool TryCreateLoadingWidget(UWorld* LoadedWorld);
	void SetLoadingProgressValue(float NewProgress);
	void PushLoadingProgressToWidget() const;
	void FinalizeLoadingProgress();
	void HandleLoadingCompletionDelayElapsed();
	void HideLoadingWidget();
	void HandleStartGameLoadingTransitionTimeout();
	void PollRoomStreamingReady();
	TSubclassOf<UUserWidget> ResolveLobbyWidgetClass(UWorld* LoadedWorld) const;
	TSubclassOf<UUserWidget> ResolveLoadingWidgetClass();
	void EnsureLocalRoomLevelInstance(UWorld* LoadedWorld);
	FString ResolveRoomCodeForInGameWorld(UWorld* LoadedWorld) const;

	UPROPERTY(Transient)
	TObjectPtr<ULevelStreamingDynamic> LocalRoomStreamingLevel;

	UPROPERTY(Transient)
	FString LocalRoomStreamingRoomCode;

	float LoadingProgress = 0.0f;
	float LoadingProgressPhaseStartTime = 0.0f;
	float RoomStreamingReadyTime = 0.0f;
	float LoadingCompletionDelayStartTime = 0.0f;
	float LoadingCompletionDelayStartProgress = 0.97f;
	float FinalizeLoadingProgressStartTime = 0.0f;
	float FinalizeLoadingProgressStartValue = 0.9f;
	float FinalizeLoadingProgressTarget = 1.0f;

	bool bStartGameLoadingTransitionActive = false;
	bool bWaitingForRoomStreamingReady = false;
	bool bRoomStreamingReady = false;
	bool bLocalGameplayPawnReady = false;
	bool bLoadingCompletionDelayActive = false;

	FTimerHandle LobbyWidgetRetryTimerHandle;
	FTimerHandle LoadingWidgetRetryTimerHandle;
	FTimerHandle StartGameLoadingTransitionTimeoutTimerHandle;
	FTimerHandle FinalizeLoadingProgressTimerHandle;
	FTimerHandle LoadingCompletionDelayTimerHandle;
	FTimerHandle RoomStreamingReadyPollTimerHandle;

	bool bFallbackLobbyBindingsActive = false;
	int32 FallbackLobbyPendingAction = 0;
};
