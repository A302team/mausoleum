// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "MyPlayerController.generated.h"

/**
 Player Controller
 IMC 활성화
 IMC를 Uproperty로 들고 있음, beginPlay에서 property를 호출하여 함수 바인딩
 *
 */

class APlayerState;
class UBaseEvent;
class UBaseGroupEvent;
class UButton;
class UCheckBox;
class UComboBoxString;
class UImage;
class UInputMappingContext;
class UPersonalEventWidget;
class USlider;
class USoundClass;
class USoundMix;
class UTextBlock;
class UTexture2D;
class UUserWidget;
class UVoteClickableUserWidget;
class UWidget;

UCLASS()
class A302_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMyPlayerController();
	bool UpdateQuickSlotItemName(int32 SlotIndex, const FText& ItemName);
	bool UpdateQuickSlotItemVisual(int32 SlotIndex, const FText& ItemName, UTexture2D* ItemIcon);
	void UpdateQuickSlotSelectionVisual(int32 SelectedSlotIndex);
	bool UpdateShieldCountText(int32 ShieldCount);
	bool UpdateMaliceCountText(int32 MaliceCount);
	bool UpdateItemTimerText(float RemainingSeconds);
	void SetItemTimerVisible(bool bVisible);
	void ToggleInGameSettingMenu();
	bool IsInGameSettingMenuOpen() const;
	void ShowPublicMaliceAnnouncement(const FString& PlayerName, int32 MaliceCount);
	float GetMouseSensitivityMultiplier() const;

	UFUNCTION(Client, Reliable)
	void Client_ShowTitleCard(const FText& Title, const FText& Context, float DisplaySeconds);

	UFUNCTION(Client, Reliable)
	void Client_HideTitleCard();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Event")
	TSubclassOf<UPersonalEventWidget> PersonalEventWidgetClass;

	UPROPERTY()
	TObjectPtr<UPersonalEventWidget> PersonalEventWidgetInstance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Event")
	TSubclassOf<UUserWidget> InspectMaliceWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> InspectMaliceWidgetInstance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Event")
	TSubclassOf<UUserWidget> GroupEventVoteWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> GroupEventVoteWidgetInstance;

	UPROPERTY()
	TObjectPtr<UBaseEvent> ActivePersonalEvent;

	UPROPERTY()
	TObjectPtr<UBaseGroupEvent> ActiveGroupEvent;
	
	UFUNCTION(Client, Reliable)
	void Client_ReceiveSystemMessage(const FString& Message);

	UFUNCTION(Client, Reliable)
	void Client_ShowPersonalEvent(FName EventID, const FText& Title, const FText& Description, const TArray<FText>& Choices);

	UFUNCTION(Client, Reliable)
	void Client_ShowInspectMaliceSelectionWidget();

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "PersonalEvent")
	void Server_ResolvePersonalEvent(FName EventID, int32 ChoiceIndex);

	UFUNCTION(Server, Reliable)
	void Server_RegisterPlayerDisplayName(const FString& DesiredName);

	UFUNCTION(Client, Reliable)
	void Client_OpenGroupEventVote(FName EventID, const FText& EventTitle, const FText& EventDescription, float VoteDuration);

	UFUNCTION(Client, Reliable)
	void Client_FinishGroupEventVote(FName EventID, const FText& ResultText);

	UFUNCTION(Client, Reliable)
	void Client_ApplyConfiscationToLocalInventory();

	UFUNCTION(Server, Reliable)
	void Server_SubmitGroupVote(FName EventID, int32 TargetPlayerId);

	void ShowInspectMaliceSelectionWidget();

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Notification")
	TSubclassOf<UUserWidget> NotificationLogClass;
	
	UPROPERTY()
	UUserWidget* NotificationLogInstance;

	UPROPERTY()
	TObjectPtr<UUserWidget> HUDWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Event")
	TSubclassOf<UUserWidget> TitleCardWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> TitleCardWidgetInstance;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> QuickSlotBarClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> QuickSlotBarWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> InGameSettingClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> InGameSettingWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	int32 MappingPriority = 0;

private:
	UUserWidget* FindQuickSlotWidget(int32 SlotIndex) const;
	UTextBlock* FindQuickSlotItemNameText(int32 SlotIndex) const;
	UImage* FindQuickSlotItemIconImage(int32 SlotIndex) const;
	UImage* FindQuickSlotItemSelectedImage(int32 SlotIndex) const;
	UButton* FindInspectMaliceButton(const FName& WidgetName) const;
	UTextBlock* FindInspectMaliceText(const FName& WidgetName) const;
	UTextBlock* FindGroupEventVoteText(const FName& WidgetName) const;
	UVoteClickableUserWidget* FindVoteUserSlot(int32 SlotIndex) const;
	UWidget* FindPublicMaliceAnnouncementWidget() const;
	UTextBlock* FindTitleCardText(const FName& WidgetName) const;
	UTextBlock* FindPublicMaliceAnnouncementText(const FName& WidgetName) const;
	UTextBlock* FindShieldCountText() const;
	UTextBlock* FindMaliceCountText() const;
	UTextBlock* FindItemTimerText() const;
	void InitializeQuickSlotVisualState();
	void InitializeInGameSettingWidget();
	void InitializeInspectMaliceWidget();
	void InitializeGroupEventVoteWidget();
	void PopulateInspectMaliceSelectionWidget();
	void ResetInspectMaliceSelectionWidget();
	void SetInspectMaliceResultVisible(bool bVisible);
	void HideInspectMaliceSelectionWidget();
	void PopulateGroupEventVoteCandidates();
	void UpdateGroupEventVoteTimerDisplay();
	void TickGroupEventVoteCountdown();
	void HandleLocalGroupVoteSelection(int32 TargetPlayerId);
	void DisableGroupVoteInteractions();
	void CloseGroupEventVoteWidget();
	void ApplyInspectMaliceSelection(int32 EntryIndex);
	int32 QueryPlayerMaliceCount(const APlayerState* TargetPlayerState) const;
	FString ResolvePlayerDisplayName(const APlayerState* TargetPlayerState) const;
	void SetPublicMaliceAnnouncementVisible(bool bVisible);
	void HidePublicMaliceAnnouncement();
	void HideTitleCard();
	void OpenInGameSettingMenu();
	void CloseInGameSettingMenu();
	void SyncVideoSettingsToCurrent();
	void SyncResolutionComboToCurrent();
	void SyncFullscreenModeComboToCurrent();
	void SyncFrameLimitComboToCurrent();
	void SyncVSyncCheckBoxToCurrent();
	void SyncMouseSensitivitySliderToCurrent();
	void SyncAudioSlidersToCurrent();
	void EnsureVideoSettingOptions();
	void LoadMouseSensitivitySetting();
	void SaveMouseSensitivitySetting(float NewValue);
	void LoadAudioSettings();
	void SaveAudioSettings();
	void ApplyAudioSettings();
	bool TryParseResolutionString(const FString& InOption, FIntPoint& OutResolution) const;
	bool TryParseFullscreenModeString(const FString& InOption, EWindowMode::Type& OutWindowMode) const;
	bool TryParseFrameLimitString(const FString& InOption, float& OutFrameLimit) const;

	UFUNCTION()
	void OnResolutionApplyClicked();

	UFUNCTION()
	void OnMouseSensitivitySliderChanged(float NewValue);

	UFUNCTION()
	void OnExitClicked();

	UFUNCTION()
	void OnInspectMalicePlayer1Clicked();

	UFUNCTION()
	void OnInspectMalicePlayer2Clicked();

	UFUNCTION()
	void OnInspectMalicePlayer3Clicked();

	UFUNCTION()
	void OnInspectMalicePlayer4Clicked();

	UFUNCTION()
	void OnInspectMalicePlayer5Clicked();

	UPROPERTY()
	TObjectPtr<UComboBoxString> ResolutionComboBox = nullptr;

	UPROPERTY()
	TObjectPtr<UButton> ResolutionApplyBtn = nullptr;

	UPROPERTY()
	TObjectPtr<UButton> ExitBtn = nullptr;

	UPROPERTY()
	TObjectPtr<UComboBoxString> FullscreenModeComboBox = nullptr;

	UPROPERTY()
	TObjectPtr<UComboBoxString> FrameLimitComboBox = nullptr;

	UPROPERTY()
	TObjectPtr<UCheckBox> VSyncCheckBox = nullptr;

	UPROPERTY()
	TObjectPtr<USlider> MouseSensitivitySlider = nullptr;

	UPROPERTY()
	float MouseSensitivityMultiplier = 1.0f;

	UPROPERTY()
	TObjectPtr<USlider> MasterVolumeSlider = nullptr;

	UPROPERTY()
	TObjectPtr<USlider> BGMVolumeSlider = nullptr;

	UPROPERTY()
	TObjectPtr<USlider> SFXVolumeSlider = nullptr;

	UPROPERTY()
	TObjectPtr<USlider> InterfaceVolumeSlider = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Audio")
	TObjectPtr<USoundClass> MasterSoundClass = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Audio")
	TObjectPtr<USoundClass> BGMSoundClass = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Audio")
	TObjectPtr<USoundClass> SFXSoundClass = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Audio")
	TObjectPtr<USoundClass> InterfaceSoundClass = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USoundMix> RuntimeAudioSettingsMix = nullptr;

	UPROPERTY()
	float MasterVolumeValue = 1.0f;

	UPROPERTY()
	float BGMVolumeValue = 1.0f;

	UPROPERTY()
	float SFXVolumeValue = 1.0f;

	UPROPERTY()
	float InterfaceVolumeValue = 1.0f;

	FTimerHandle InspectMaliceHideTimerHandle;
	FTimerHandle PublicMaliceAnnouncementHideTimerHandle;
	FTimerHandle TitleCardHideTimerHandle;
	FTimerHandle GroupEventVoteCountdownHandle;
	FTimerHandle GroupEventVoteCloseHandle;

	UPROPERTY()
	TObjectPtr<UTextBlock> GroupEventVoteTimerText = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> GroupEventVoteTitleText = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> GroupEventVoteDescriptionText = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UVoteClickableUserWidget>> VoteUserSlotWidgets;

	FName ActiveGroupVoteEventID = NAME_None;
	int32 GroupEventVoteRemainingSeconds = 0;
	bool bHasSubmittedGroupVote = false;

	UPROPERTY()
	TArray<TObjectPtr<APlayerState>> InspectMaliceSelectablePlayers;
};
