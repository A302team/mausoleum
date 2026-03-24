// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameMode/A302GameState.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "MyPlayerController.generated.h"

/**
 Player Controller
 IMC 활성화
 IMC를 Uproperty로 들고 있음, beginPlay에서 property를 호출하여 함수 바인딩
 *
 */

class UBaseEvent;
class UBaseGroupEvent;
class UInputMappingContext;
class UPersonalEventWidget;
class UPlayerEventComponent;
class UTextBlock;
class UUserWidget;
class UBasePersonalEvent;
class UMaliceBGMComponent; // Added
class UGameBGMComponent;  // Added
class UCursedSwordBGMComponent; // Added
class AA302GameState;

UCLASS()
class A302SHARED_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMyPlayerController();
	virtual void ToggleInGameSettingMenu();
	bool IsInGameSettingMenuOpen() const;
	float GetMouseSensitivityMultiplier() const;
	virtual void ShowPublicMaliceAnnouncement(const FString& PlayerName, int32 MaliceCount);
	virtual void SetActivePersonalEvent(UBaseEvent* Event);
	virtual void SetActiveGroupEvent(UBaseGroupEvent* Event);
	virtual void ShowPersonalEvent(FName EventID, const FText& Title, const FText& Description, const TArray<FText>& Choices);
	virtual void ShowInspectMaliceSelectionWidget();
	virtual void ShowInspectMaliceSelectionWidgetWithConfig(float SelectionTimeoutSeconds, float ResultDisplaySeconds);
	virtual void OpenGroupEventVote(FName EventID, const FText& EventTitle, const FText& EventDescription, float VoteDuration);
	virtual void FinishGroupEventVote(FName EventID, const FText& ResultText);
	virtual void ApplyConfiscationToLocalInventory();
	virtual void UpdateShieldCount(int32 ShieldCount);
	virtual void UpdateMaliceCount(int32 MaliceCount);
	virtual void UpdateItemTimer(float RemainingSeconds);
	virtual void SetItemTimerVisibleForClient(bool bVisible);
	virtual void ConfigureMatchTimer(float MatchStartServerTime, float DurationSeconds, bool bVisible);
	virtual void ShowResultScreen(const FText& Title, const FText& Description, float DisplaySeconds);
	virtual void ToggleVoiceChatCapture();

	// Added: Malice BGM Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Audio")
	TObjectPtr<UMaliceBGMComponent> MaliceBGMComp;
	// End Added

	// Added: Game BGM Component (GameBGM <-> MaliceBGM 상태 전환)
	UPROPERTY(VisibleAnywhere, Category = "Audio")
	TObjectPtr<UGameBGMComponent> GameBGMComp;
	// End Added

	// Added: CursedSword BGM Component (최우선 BGM 관리)
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCursedSwordBGMComponent> CursedSwordBGMComp;
	// End Added

	// UI 위젯 속성은 모두 AA302GameHUD로 이동되었습니다.

	UFUNCTION(Client, Reliable)
	void Client_ShowInspectMaliceSelectionWidget();

	UFUNCTION(Client, Reliable)
	void Client_ShowInspectMaliceSelectionWidgetWithConfig(float SelectionTimeoutSeconds, float ResultDisplaySeconds);

	UFUNCTION(Client, Reliable)
	void Client_ShowTitleCard(const FText& Title, const FText& Context, float DisplaySeconds);

	UFUNCTION(Client, Reliable)
	void Client_HideTitleCard();

	UFUNCTION(Client, Reliable)
	void Client_ReceiveSystemMessage(const FString& SystemMessage);

	UFUNCTION(Client, Reliable)
	void Client_ShowPublicMaliceAnnouncement(const FString& PlayerName, int32 MaliceCount);

	UFUNCTION(Client, Reliable)
	void Client_UpdateItemTimer(float RemainingSeconds);

	UFUNCTION(Client, Reliable)
	void Client_SetItemTimerVisible(bool bVisible);

	UFUNCTION(Client, Reliable)
	void Client_ConfigureMatchTimer(float MatchStartServerTime, float DurationSeconds, bool bVisible);

	UFUNCTION(Client, Reliable)
	void Client_ShowResultScreen(const FText& Title, const FText& Description, float DisplaySeconds);

	UFUNCTION(Client, Reliable)
	void Client_RemoveQuickSlotItemByServer(int32 SlotIndex, FName ExpectedItemId);


	UFUNCTION(Server, Reliable)
	void Server_RegisterPlayerDisplayName(const FString& DesiredName);

	UFUNCTION(BlueprintCallable, Category = "Event")
	UPlayerEventComponent* GetPlayerEventComponent() const { return PlayerEventComponent; }

protected:
	virtual void BeginPlay() override;
    virtual void OnRep_Pawn() override;
	virtual void AcknowledgePossession(APawn* P) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPlayerEventComponent> PlayerEventComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	int32 MappingPriority = 0;

private:
	bool ShouldAttemptGameplayHUDInitialization() const;
	void TryInitializeInGameHUD();
	void PollDeferredHUDInitialization();
	void ApplyMatchTimerConfigToHUD();
    void BindToReplicatedGamePhase();
    void HandleReplicatedGamePhaseChanged(EGamePhase PreviousPhase, EGamePhase NewPhase, float PhaseChangedServerTime);
    void QueuePhaseTransition(EGamePhase NewPhase, float PhaseChangedServerTime);
    void FlushQueuedPhaseTransition();
	void NotifyLocalGameplayPawnReady();
    void EnsureLocalVoiceComponent();
	bool bInGameHUDInitialized = false;
	bool bHasPendingMatchTimerConfig = false;
	bool bHasQueuedPhaseTransition = false;
	bool bPendingMatchTimerVisible = false;
	float PendingMatchTimerStartServerTime = 0.0f;
	float PendingMatchTimerDurationSeconds = 0.0f;
	float QueuedPhaseChangedServerTime = 0.0f;
	EGamePhase QueuedPhaseTransition = EGamePhase::Phase0;
	TWeakObjectPtr<AA302GameState> BoundGameState;
	FTimerHandle DeferredHUDInitTimerHandle;
	FTimerHandle VoiceRoomCodeRetryTimerHandle;
	int32 VoiceRoomCodeRetryCount = 0;
};
