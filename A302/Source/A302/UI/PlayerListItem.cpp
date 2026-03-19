// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/PlayerListItem.h"
#include "UI/NicknameUiConstants.h"
#include "Components/TextBlock.h"

void UPlayerListItem::NativeConstruct()
{
    Super::NativeConstruct();
}

void UPlayerListItem::SetPlayerName(const FString& PlayerName)
{
    if (Text_PlayerName) {
        const FString ClampedPlayerName = PlayerName.Len() > A302UI::MaxNicknameUiLen ? PlayerName.Left(A302UI::MaxNicknameUiLen) : PlayerName;
        Text_PlayerName->SetText(FText::FromString(ClampedPlayerName));
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

