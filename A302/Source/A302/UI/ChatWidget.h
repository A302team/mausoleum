// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChatWidget.generated.h"

/**
 * 
 */
UCLASS()
class A302_API UChatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UScrollBox> ScrollBox_Message;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UEditableTextBox> Input_ChatMessage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> Btn_Send;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class UChatMessageItem> ChatMessageItemClass;

	UFUNCTION()
	void OnSendClicked();

	UFUNCTION()
	void OnChatMessageReceived(const FString& PlayerName, const FString& Message);

	UFUNCTION()
	void OnInputCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	UFUNCTION()
	void FocusChatInput();

private:
	TObjectPtr<class UA302GameInstance> GI;
	TObjectPtr<class AA302GameMode> InGameGameMode;

	void SendMessage();
	void AddMessage(const FString& PlayerName, const FString& Message);	
};
