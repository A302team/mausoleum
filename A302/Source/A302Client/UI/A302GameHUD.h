#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "A302GameHUD.generated.h"

class UPersonalEventWidget;
class UUserWidget;

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

	void InitializeChatWidget();

	UFUNCTION()
	void InitializeClientInGameWidgets();

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UPlayerHUDComponent> PlayerHUDComponent;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	TObjectPtr<UUserWidget> ChatWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> TitleCardWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> ResultWidgetInstance;

private:
	FTimerHandle TitleCardHideTimerHandle;
};
