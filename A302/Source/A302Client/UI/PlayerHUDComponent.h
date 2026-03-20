#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameFramework/GameUserSettings.h"
#include "PlayerHUDComponent.generated.h"

class AMyPlayerController;
class APlayerState;
class UButton;
class UCheckBox;
class UComboBoxString;
class UPersonalEventWidget;
class UQuickSlotComponent;
class USlider;
class USoundClass;
class USoundMix;
class UTextBlock;
class UTexture2D;
class UUserWidget;
class UWidget;
class UItemDefinition;
class UGroupEventHUDComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302CLIENT_API UPlayerHUDComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerHUDComponent();

	void ShowPersonalEventUI(TSubclassOf<UPersonalEventWidget> PersonalEventWidgetClass, FName EventID, const FText& EventTitle, const FText& EventDescription, const TArray<FText>& Choices);
	void ShowGroupEventVoteUI(TSubclassOf<UUserWidget> GroupEventVoteWidgetClass, FName EventID, const FText& EventTitle, const FText& EventDescription, float VoteDuration);
	void FinishGroupEventVoteUI(FName EventID, const FText& ResultText);
	void CloseGroupEventVoteUI();
	void InitializeInGameHUD(TSubclassOf<UUserWidget> InQuickSlotBarClass, TSubclassOf<UUserWidget> InInGameSettingClass, TSubclassOf<UUserWidget> InInspectMaliceWidgetClass);
	void RefreshQuickSlotBinding();
	void ToggleInGameSettingMenu();
	bool IsInGameSettingMenuOpen() const;
	float GetMouseSensitivityMultiplier() const;
	void ShowPublicMaliceAnnouncement(const FString& PlayerName, int32 MaliceCount);
	void ShowInspectMaliceSelectionWidget();
	void ShowInspectMaliceSelectionWidgetWithConfig(float SelectionTimeoutSeconds, float ResultDisplaySeconds);
	bool UpdateShieldCountText(int32 ShieldCount);
	bool UpdateMaliceCountText(int32 MaliceCount);
	bool UpdateItemTimerText(float RemainingSeconds);
	void SetItemTimerVisible(bool bVisible);

private:
	AMyPlayerController* GetOwnerController() const;
	FString ResolvePlayerDisplayName(const APlayerState* TargetPlayerState) const;
	void EnsureGroupEventHUDComponent();
	void InitializeQuickSlotWidget();
	void InitializeQuickSlotVisualState();
	void BindQuickSlotComponent();
	void SyncQuickSlotUIFromComponent();
	void HandleQuickSlotSelectionChanged(int32 SelectedSlotIndex);
	void HandleQuickSlotItemChanged(int32 SlotIndex, const UItemDefinition* ItemDefinition);
	UUserWidget* FindQuickSlotWidget(int32 SlotIndex) const;
	UTextBlock* FindQuickSlotItemNameText(int32 SlotIndex) const;
	UTextBlock* FindShieldCountText() const;
	UTextBlock* FindMaliceCountText() const;
	UTextBlock* FindItemTimerText() const;
	UWidget* FindPublicMaliceAnnouncementWidget() const;
	UTextBlock* FindPublicMaliceAnnouncementText(const FName& WidgetName) const;
	void SetPublicMaliceAnnouncementVisible(bool bVisible);
	void HidePublicMaliceAnnouncement();
	bool UpdateQuickSlotItemVisual(int32 SlotIndex, const FText& ItemName, UTexture2D* ItemIcon);
	void UpdateQuickSlotSelectionVisual(int32 SelectedSlotIndex);
	void InitializeInGameSettingWidget();
	void OpenInGameSettingMenu();
	void CloseInGameSettingMenu();
	void EnsureVideoSettingOptions();
	void SyncVideoSettingsToCurrent();
	void SyncResolutionComboToCurrent();
	void SyncFullscreenModeComboToCurrent();
	void SyncFrameLimitComboToCurrent();
	void SyncVSyncCheckBoxToCurrent();
	bool TryParseResolutionString(const FString& InOption, FIntPoint& OutResolution) const;
	bool TryParseFullscreenModeString(const FString& InOption, EWindowMode::Type& OutWindowMode) const;
	bool TryParseFrameLimitString(const FString& InOption, float& OutFrameLimit) const;
	void LoadMouseSensitivitySetting();
	void SaveMouseSensitivitySetting(float NewValue);
	void SyncMouseSensitivitySliderToCurrent();
	void LoadAudioSettings();
	void SaveAudioSettings();
	void SyncAudioSlidersToCurrent();
	void ApplyAudioSettings();
	void InitializeInspectMaliceWidget();
	void PopulateInspectMaliceSelectionWidget();
	void ResetInspectMaliceSelectionWidget();
	void SetInspectMaliceResultVisible(bool bVisible);
	void HideInspectMaliceSelectionWidget();
	void ApplyInspectMaliceSelection(int32 EntryIndex);
	void RetryPopulateInspectMaliceSelectionWidget();
	void HandleInspectMaliceSelectionTimeout();
	void SetInspectMaliceSelectionButtonsEnabled(bool bEnabled);
	void StartInspectMaliceItemTimer(float DurationSeconds);
	void StopInspectMaliceItemTimer();
	void TickInspectMaliceItemTimer();
	int32 QueryPlayerMaliceCount(const APlayerState* TargetPlayerState) const;
	UButton* FindInspectMaliceButton(const FName& WidgetName) const;
	UTextBlock* FindInspectMaliceText(const FName& WidgetName) const;

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

	UPROPERTY(Transient)
	TObjectPtr<UPersonalEventWidget> PersonalEventWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UGroupEventHUDComponent> GroupEventHUDComponent;

	FTimerHandle PublicMaliceAnnouncementHideTimerHandle;
	FTimerHandle InspectMaliceHideTimerHandle;
	FTimerHandle InspectMaliceSelectionTimeoutHandle;
	FTimerHandle InspectMalicePopulateRetryHandle;
	FTimerHandle InspectMaliceItemTimerTickHandle;

	float InspectMaliceItemTimerEndTime = 0.0f;
	bool bInspectMaliceItemTimerBaselineCaptured = false;
	bool bInspectMaliceItemTimerWasVisible = false;

	UPROPERTY(Transient)
	TSubclassOf<UUserWidget> QuickSlotBarClass;

	UPROPERTY(Transient)
	TSubclassOf<UUserWidget> InGameSettingClass;

	UPROPERTY(Transient)
	TSubclassOf<UUserWidget> InspectMaliceWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> QuickSlotBarWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> InGameSettingWidget;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> InspectMaliceWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UComboBoxString> ResolutionComboBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ResolutionApplyBtn = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UButton> ExitBtn = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UComboBoxString> FullscreenModeComboBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UComboBoxString> FrameLimitComboBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> VSyncCheckBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USlider> MouseSensitivitySlider = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USlider> MasterVolumeSlider = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USlider> BGMVolumeSlider = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<USlider> SFXVolumeSlider = nullptr;

	UPROPERTY(Transient)
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

	UPROPERTY(Transient)
	float MasterVolumeValue = 1.0f;

	UPROPERTY(Transient)
	float BGMVolumeValue = 1.0f;

	UPROPERTY(Transient)
	float SFXVolumeValue = 1.0f;

	UPROPERTY(Transient)
	float InterfaceVolumeValue = 1.0f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<APlayerState>> InspectMaliceSelectablePlayers;

	UPROPERTY(Transient)
	float InspectMaliceSelectionTimeoutSeconds = 10.0f;

	UPROPERTY(Transient)
	float InspectMaliceResultDisplaySeconds = 3.0f;

	UPROPERTY(Transient)
	bool bInspectMaliceSelectionConsumed = false;

	UPROPERTY(Transient)
	TObjectPtr<UQuickSlotComponent> BoundQuickSlotComponent = nullptr;

	UPROPERTY(Transient)
	float MouseSensitivityMultiplier = 1.0f;
};
