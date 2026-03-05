// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WaitingRoomWidget.generated.h"

/**
 *
 */
UCLASS()
class A302_API UWaitingRoomWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry &InGeometry, const FKeyEvent &InKeyEvent) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UChatWidget> ChatWidget;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> Text_RoomCode;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UScrollBox> ScrollBox_Players;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> Btn_Ready;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> Btn_StartGame;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> Btn_Leave;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UPlayerListItem> PlayerListItemClass;

	// 방 코드
	UFUNCTION()
	void SetRoomCode(const FString &RoomCode);

	// 플레이어 입장
	UFUNCTION()
	void OnPlayerEntered(const FString &PlayerName);

	// 플레이어 레디
	UFUNCTION()
	void OnPlayerReady(const FString &PlayerName);

	// 플레이어 퇴장
	UFUNCTION()
	void OnPlayerLeft(const FString &PlayerName);

	UFUNCTION()
	void OnReadyClicked();

	UFUNCTION()
	void OnStartGameClicked();

	UFUNCTION()
	void OnLeaveClicked();

private:
	TObjectPtr<class ALobbyGameMode> LobbyGameMode;

	TMap<FString, TObjectPtr<class UPlayerListItem>> PlayerItems;
};
