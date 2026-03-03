// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerListItem.generated.h"

/**
 * 
 */
UCLASS()
class A302_API UPlayerListItem : public UUserWidget
{
	GENERATED_BODY()
	
public:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<class UTextBlock> Text_PlayerName;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<class UTextBlock> Text_ReadyStatus;

    void SetPlayerName(const FString& PlayerName);
    void SetReady(bool bReady);
};
