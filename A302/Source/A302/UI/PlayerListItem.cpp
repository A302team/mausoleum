// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/PlayerListItem.h"
#include "Components/TextBlock.h"

namespace
{
    constexpr int32 MaxNicknameUiLen = 7;
}

void UPlayerListItem::NativeConstruct()
{
    Super::NativeConstruct();
}

void UPlayerListItem::SetPlayerName(const FString& PlayerName)
{
    if (Text_PlayerName) {
        const FString ClampedPlayerName = PlayerName.Len() > MaxNicknameUiLen ? PlayerName.Left(MaxNicknameUiLen) : PlayerName;
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

