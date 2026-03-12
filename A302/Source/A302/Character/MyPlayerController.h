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
class UComboBoxString;
class UImage;
class UInputMappingContext;
class UPersonalEventWidget;
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
	void OpenInGameSettingMenu();
	void CloseInGameSettingMenu();
	void SyncResolutionComboToCurrent();
	bool TryParseResolutionString(const FString& InOption, FIntPoint& OutResolution) const;

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

	UPROPERTY()
	TObjectPtr<UComboBoxString> ResolutionComboBox = nullptr;

	UPROPERTY()
	TObjectPtr<UButton> ResolutionApplyBtn = nullptr;

	UPROPERTY()
	TObjectPtr<UButton> ExitBtn = nullptr;

	FTimerHandle InspectMaliceHideTimerHandle;
	FTimerHandle PublicMaliceAnnouncementHideTimerHandle;
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
