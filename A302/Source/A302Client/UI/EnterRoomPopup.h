// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EnterRoomPopup.generated.h"

/**
 * 
 */
UCLASS()
class A302CLIENT_API UEnterRoomPopup : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UEditableTextBox> Input_RoomCode;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> Btn_Confirm;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> Btn_Cancel;

	UFUNCTION()
	void OnConfirmClicked();

	UFUNCTION()
	void OnCancelClicked();

private:
	TObjectPtr<class UA302GameInstance> GI;	
};
