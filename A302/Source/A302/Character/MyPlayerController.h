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

class UInputMappingContext;
class UUserWidget;
class UTextBlock;
class UWidget;
class UImage;
class UTexture2D;
class UComboBoxString;
class UButton;
class UBaseEvent;
class UPersonalEventWidget;

UCLASS()
class A302_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMyPlayerController();
	bool UpdateQuickSlotItemName(int32 SlotIndex, const FText &ItemName);
	bool UpdateQuickSlotItemVisual(int32 SlotIndex, const FText &ItemName, UTexture2D *ItemIcon);
	void UpdateQuickSlotSelectionVisual(int32 SelectedSlotIndex);
	bool UpdateShieldCountText(int32 ShieldCount);
	bool UpdateMaliceCountText(int32 MaliceCount);
	bool UpdateItemTimerText(float RemainingSeconds);
	void SetItemTimerVisible(bool bVisible);
	void ToggleInGameSettingMenu();
	bool IsInGameSettingMenuOpen() const;
	void ShowPublicMaliceAnnouncement(const FString& PlayerName, int32 MaliceCount);
	
	// Event UI 위젯 클래스 (블루프린트에서 세팅)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Event")
	TSubclassOf<class UPersonalEventWidget> PersonalEventWidgetClass;

	UPROPERTY()
	TObjectPtr<UPersonalEventWidget> PersonalEventWidgetInstance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Event")
	TSubclassOf<UUserWidget> InspectMaliceWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> InspectMaliceWidgetInstance;

	// 서버가 이 플레이어의 현재 진행 중인 이벤트를 기억해두기 위한 포인터
	UPROPERTY()
	TObjectPtr<UBaseEvent> ActivePersonalEvent;

	// 클라이언트의 화면에 이벤트를 띄우는 함수
	UFUNCTION(Client, Reliable)
    void Client_ShowPersonalEvent(FName EventID, const FText& Title, const FText& Description, const TArray<FText>& Choices);

	// 클라이언트가 확인 버튼을 눌렀을 때 서버로 알리는 함수
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "PersonalEvent")
	void Server_ResolvePersonalEvent(FName EventID, int32 ChoiceIndex);

	void ShowInspectMaliceSelectionWidget();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> QuickSlotBarClass;

	// 런타임에 생성된 위젯 인스턴스(화면에 띄운 객체)
	UPROPERTY()
	TObjectPtr<UUserWidget> QuickSlotBarWidget;

	UPROPERTY(EditDefaultsOnly, Category="UI")
	TSubclassOf<UUserWidget> InGameSettingClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> InGameSettingWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext *DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	int32 MappingPriority = 0;

private:
	UUserWidget *FindQuickSlotWidget(int32 SlotIndex) const;
	class UTextBlock *FindQuickSlotItemNameText(int32 SlotIndex) const;
	class UImage *FindQuickSlotItemIconImage(int32 SlotIndex) const;
	class UImage *FindQuickSlotItemSelectedImage(int32 SlotIndex) const;
	class UButton *FindInspectMaliceButton(const FName& WidgetName) const;
	class UTextBlock *FindInspectMaliceText(const FName& WidgetName) const;
	class UWidget *FindPublicMaliceAnnouncementWidget() const;
	class UTextBlock *FindPublicMaliceAnnouncementText(const FName& WidgetName) const;
	class UTextBlock *FindShieldCountText() const;
	class UTextBlock *FindMaliceCountText() const;
	class UTextBlock *FindItemTimerText() const;
	void InitializeQuickSlotVisualState();
	void InitializeInGameSettingWidget();
	void InitializeInspectMaliceWidget();
	void ResetInspectMaliceSelectionWidget();
	void SetInspectMaliceResultVisible(bool bVisible);
	void HideInspectMaliceSelectionWidget();
	int32 QueryDummy1MaliceCount() const;
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
	void OnInspectMaliceDummy1Clicked();

	UPROPERTY()
	TObjectPtr<UComboBoxString> ResolutionComboBox = nullptr;

	UPROPERTY()
	TObjectPtr<UButton> ResolutionApplyBtn = nullptr;

	UPROPERTY()
	TObjectPtr<UButton> ExitBtn = nullptr;

	FTimerHandle InspectMaliceHideTimerHandle;
	FTimerHandle PublicMaliceAnnouncementHideTimerHandle;

public:
};
