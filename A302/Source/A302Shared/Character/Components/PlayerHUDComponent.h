#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerHUDComponent.generated.h"

class AMyPlayerController;
class APlayerState;
class UButton;
class UComboBoxString;
class UPersonalEventWidget;
class UQuickSlotComponent;
class UTextBlock;
class UTexture2D;
class UUserWidget;
class UVoteClickableUserWidget;
class UWidget;
class UItemDefinition;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302SHARED_API UPlayerHUDComponent : public UActorComponent
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
	void ShowPublicMaliceAnnouncement(const FString& PlayerName, int32 MaliceCount);
	void ShowInspectMaliceSelectionWidget();
	bool UpdateShieldCountText(int32 ShieldCount);
	bool UpdateMaliceCountText(int32 MaliceCount);
	bool UpdateItemTimerText(float RemainingSeconds);
	void SetItemTimerVisible(bool bVisible);

private:
	AMyPlayerController* GetOwnerController() const;
	void InitializeGroupEventVoteWidget(TSubclassOf<UUserWidget> GroupEventVoteWidgetClass);
	void PopulateGroupEventVoteCandidates();
	void UpdateGroupEventVoteTimerDisplay();
	void TickGroupEventVoteCountdown();
	void DisableGroupVoteInteractions();
	void HandleLocalGroupVoteSelection(int32 TargetPlayerId);
	UTextBlock* FindGroupEventVoteText(const FName& WidgetName) const;
	UVoteClickableUserWidget* FindVoteUserSlot(int32 SlotIndex) const;
	FString ResolvePlayerDisplayName(const APlayerState* TargetPlayerState) const;
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
	void SyncResolutionComboToCurrent();
	bool TryParseResolutionString(const FString& InOption, FIntPoint& OutResolution) const;
	void InitializeInspectMaliceWidget();
	void PopulateInspectMaliceSelectionWidget();
	void ResetInspectMaliceSelectionWidget();
	void SetInspectMaliceResultVisible(bool bVisible);
	void HideInspectMaliceSelectionWidget();
	void ApplyInspectMaliceSelection(int32 EntryIndex);
	int32 QueryPlayerMaliceCount(const APlayerState* TargetPlayerState) const;
	UButton* FindInspectMaliceButton(const FName& WidgetName) const;
	UTextBlock* FindInspectMaliceText(const FName& WidgetName) const;

	UFUNCTION()
	void OnResolutionApplyClicked();

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
	TObjectPtr<UUserWidget> GroupEventVoteWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GroupEventVoteTimerText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GroupEventVoteTitleText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GroupEventVoteDescriptionText = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UVoteClickableUserWidget>> VoteUserSlotWidgets;

	FTimerHandle GroupEventVoteCountdownHandle;
	FTimerHandle GroupEventVoteCloseHandle;
	FTimerHandle PublicMaliceAnnouncementHideTimerHandle;
	FTimerHandle InspectMaliceHideTimerHandle;

	FName ActiveGroupVoteEventID = NAME_None;
	int32 GroupEventVoteRemainingSeconds = 0;
	bool bHasSubmittedGroupVote = false;

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
	TArray<TObjectPtr<APlayerState>> InspectMaliceSelectablePlayers;

	UPROPERTY(Transient)
	TObjectPtr<UQuickSlotComponent> BoundQuickSlotComponent = nullptr;
};
