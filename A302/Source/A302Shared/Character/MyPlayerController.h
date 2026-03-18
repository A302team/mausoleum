// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Interface/A302ClientEventBridge.h"
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
class UPlayerHUDComponent;
class UTextBlock;
class UUserWidget;

UCLASS()
class A302SHARED_API AMyPlayerController : public APlayerController, public IA302ClientEventBridge
{
	GENERATED_BODY()

public:
	AMyPlayerController();
	virtual void ToggleInGameSettingMenu() override;
	bool IsInGameSettingMenuOpen() const;
	float GetMouseSensitivityMultiplier() const;
	virtual void ShowPublicMaliceAnnouncement(const FString& PlayerName, int32 MaliceCount) override;
	virtual void SetActivePersonalEvent(UBaseEvent* Event) override;
	virtual void SetActiveGroupEvent(UBaseGroupEvent* Event) override;
	virtual void ShowPersonalEvent(FName EventID, const FText& Title, const FText& Description, const TArray<FText>& Choices) override;
	virtual void ShowInspectMaliceSelectionWidget() override;
	virtual void OpenGroupEventVote(FName EventID, const FText& EventTitle, const FText& EventDescription, float VoteDuration) override;
	virtual void FinishGroupEventVote(FName EventID, const FText& ResultText) override;
	virtual void ApplyConfiscationToLocalInventory() override;
	virtual void UpdateShieldCount(int32 ShieldCount) override;
	virtual void UpdateMaliceCount(int32 MaliceCount) override;
	virtual void UpdateItemTimer(float RemainingSeconds) override;
	virtual void SetItemTimerVisibleForClient(bool bVisible) override;
	virtual void ToggleVoiceChatCapture() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Event")
	TSubclassOf<UPersonalEventWidget> PersonalEventWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Event")
	TSubclassOf<UUserWidget> InspectMaliceWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Event")
	TSubclassOf<UUserWidget> GroupEventVoteWidgetClass;

	UFUNCTION(Client, Reliable)
	void Client_ShowInspectMaliceSelectionWidget();

	UFUNCTION(Client, Reliable)
	void Client_ShowTitleCard(const FText& Title, const FText& Context, float DisplaySeconds);

	UFUNCTION(Client, Reliable)
	void Client_HideTitleCard();

	UFUNCTION(Server, Reliable)
	void Server_RegisterPlayerDisplayName(const FString& DesiredName);

	UFUNCTION(BlueprintCallable, Category = "Event")
	UPlayerEventComponent* GetPlayerEventComponent() const { return PlayerEventComponent; }

	UFUNCTION(BlueprintCallable, Category = "UI")
	UPlayerHUDComponent* GetPlayerHUDComponent() const { return PlayerHUDComponent; }

protected:
	virtual void BeginPlay() override;
    virtual void OnRep_Pawn() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPlayerEventComponent> PlayerEventComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPlayerHUDComponent> PlayerHUDComponent;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> QuickSlotBarClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> ChatWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> ChatWidgetInstance;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> InGameSettingClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Event")
	TSubclassOf<UUserWidget> TitleCardWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> TitleCardWidgetInstance;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	int32 MappingPriority = 0;

private:
	void InitializeChatWidget();
	void InitializeClientInGameWidgets();
	void HideTitleCard();
	bool IsInGameMap() const;
    void EnsureLocalVoiceComponent();

	FTimerHandle TitleCardHideTimerHandle;
};
