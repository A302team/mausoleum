// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LobbyWidget.generated.h"

/**
 * 
 */
UCLASS()
class A302_API ULobbyWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> Btn_CreateRoom;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> Btn_Ready;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> Btn_StartGame;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UEditableTextBox> Input_RoomId;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UEditableTextBox> Input_PlayerName;

	UFUNCTION()
	void OnCreateRoomClicked();

	UFUNCTION()
	void OnReadyClicked();

	UFUNCTION()
	void OnStartGameClicked();

private:
	TObjectPtr<class ALobbyGameMode> LobbyGameMode;
};
