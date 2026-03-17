// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyWidget.generated.h"

/**
 *
 */
UENUM()
enum class EPendingAction :uint8
{
	None,
	CreateRoom,
	EnterRoom,
	FindRoom
};

UCLASS()
class A302CLIENT_API ULobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> Btn_CreateRoom;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> Btn_EnterRoom;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> Btn_FindRoom;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> Btn_Exit;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UEditableTextBox> Input_PlayerName;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UEditableTextBox> Input_RoomCode;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class URoomListPopup> RoomListPopupClass;

	UPROPERTY()
	TObjectPtr<class UEnterRoomPopup> EnterRoomPopup;

	EPendingAction PendingAction = EPendingAction::None;

	UFUNCTION()
	void CheckNickname(const FString& PlayerName);

	UFUNCTION()
	void OnNicknameAvailable();

	UFUNCTION()
	void OnCreateRoomClicked();

	UFUNCTION()
	void OnEnterRoomClicked();

	UFUNCTION()
	void OnFindRoomClicked();

	UFUNCTION()
	void OnExitClicked();

	UFUNCTION()
	void OnRoomCreated(const FString &RoomCode);

private:
	TObjectPtr<class UA302GameInstance> GI;
};
