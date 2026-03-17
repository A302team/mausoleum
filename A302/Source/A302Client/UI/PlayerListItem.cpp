// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/PlayerListItem.h"
#include "Components/TextBlock.h"

void UPlayerListItem::NativeConstruct()
{
    Super::NativeConstruct();
}

void UPlayerListItem::SetPlayerName(const FString& PlayerName)
{
    if (Text_PlayerName) {
        Text_PlayerName->SetText(FText::FromString(PlayerName));
    }
}

void UPlayerListItem::SetReady(bool bReady)
{
    if (Text_ReadyStatus) {
        Text_ReadyStatus->SetText(
            FText::FromString(bReady ? TEXT("레디") : TEXT("대기"))
        );
    }
}

