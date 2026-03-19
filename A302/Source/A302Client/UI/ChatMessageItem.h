// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ChatMessageItem.generated.h"

/**
 * 
 */
UCLASS()
class A302CLIENT_API UChatMessageItem : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<class UTextBlock> Text_PlayerName;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<class UTextBlock> Text_Message;

	void SetMessage(const FString& PlayerName, const FString& Message);
};
