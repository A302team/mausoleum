#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "A302GameHUD.generated.h"

class UPersonalEventWidget;
class UBorder;
class UUserWidget;

enum class EA302PhaseTransitionState : uint8
{
	Idle,
	FadeToBlack,
	HoldBlack,
	FadeFromBlack
};

UCLASS()
class A302CLIENT_API AA302GameHUD : public AHUD
{
	GENERATED_BODY()

public:
	AA302GameHUD();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Event")
	TSubclassOf<UPersonalEventWidget> PersonalEventWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Event")
	TSubclassOf<UUserWidget> InspectMaliceWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Event")
	TSubclassOf<UUserWidget> GroupEventVoteWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Event")
	TSubclassOf<UUserWidget> TitleCardWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Event")
	TSubclassOf<UUserWidget> PhaseTransitionWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Item")
	TSubclassOf<UUserWidget> ItemDescriptionWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> QuickSlotBarClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> ChatWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> InGameSettingClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> ResultWidgetClass;

	UFUNCTION()
	void ShowTitleCard(const FText& Title, const FText& Context, float DisplaySeconds);

	UFUNCTION()
	void HideTitleCard();

	UFUNCTION()
	void StartPhaseTransition(const FText& Title, const FText& Context, float FadeOutSeconds, float HoldSeconds, float FadeInSeconds, float TitleDisplaySeconds);

	void InitializeChatWidget();

	UFUNCTION()
	void InitializeClientInGameWidgets();

	UFUNCTION()
	void RefreshQuickSlotBinding();

	UFUNCTION()
	void ToggleInGameSettingMenu();

	UFUNCTION()
	bool IsInGameSettingMenuOpen() const;

	UFUNCTION()
	float GetMouseSensitivityMultiplier() const;

	UFUNCTION()
	void ShowPublicMaliceAnnouncement(const FString& PlayerName, int32 MaliceCount);

	UFUNCTION()
	void ShowInspectMaliceSelectionWidget();

	UFUNCTION()
	void ShowInspectMaliceSelectionWidgetWithConfig(float SelectionTimeoutSeconds, float ResultDisplaySeconds);

	UFUNCTION()
	void UpdateShieldCountText(int32 ShieldCount);

	UFUNCTION()
	void UpdateMaliceCountText(int32 MaliceCount);

	UFUNCTION()
	void UpdateItemTimerText(float RemainingSeconds);

	UFUNCTION()
	void SetItemTimerVisible(bool bVisible);

	UFUNCTION()
	void ConfigureMatchTimer(float MatchStartServerTime, float DurationSeconds, bool bVisible);

	UFUNCTION()
	void ShowPersonalEvent(FName EventID, const FText& EventTitle, const FText& EventDescription, const TArray<FText>& Choices);

	UFUNCTION()
	void ShowGroupEventVote(FName EventID, const FText& EventTitle, const FText& EventDescription, float VoteDuration);

	UFUNCTION()
	void FinishGroupEventVoteUI(FName EventID, const FText& ResultText);

	UFUNCTION()
	void ShowResultScreen(const FText& Title, const FText& Description, float DisplaySeconds);

	UFUNCTION()
	void ShowItemDescription(const FText& ItemName, const FText& ItemDescription, float DisplaySeconds = 4.0f);

	UFUNCTION()
	void HideItemDescription();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UPlayerHUDComponent> PlayerHUDComponent;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TObjectPtr<UUserWidget> ChatWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> TitleCardWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> PhaseTransitionWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ResultWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ItemDescriptionWidgetInstance;

private:
	void TickPhaseTransition();
	void BeginPhaseTransitionStep(EA302PhaseTransitionState NewState, float DurationSeconds);
	void FinishPhaseTransition();
	void SetPhaseTransitionOverlayOpacity(float Opacity);
	void SetPhaseTransitionInputLocked(bool bLocked);
	UBorder* FindPhaseTransitionBlackOverlay() const;

	FTimerHandle PhaseTransitionTickTimerHandle;
	FTimerHandle PhaseTransitionInputRestoreTimerHandle;
	FTimerHandle TitleCardHideTimerHandle;
	FTimerHandle ItemDescriptionHideTimerHandle;
	EA302PhaseTransitionState PhaseTransitionState = EA302PhaseTransitionState::Idle;
	float PhaseTransitionStepDuration = 0.0f;
	float PhaseTransitionStepStartTime = 0.0f;
	float PendingPhaseHoldSeconds = 0.0f;
	float PendingPhaseFadeInSeconds = 0.0f;
	float PendingPhaseTitleDisplaySeconds = 0.0f;
	bool bPhaseTransitionInputLocked = false;
	FText PendingPhaseTransitionTitle;
	FText PendingPhaseTransitionContext;
};
