// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ChatMessageItem.h"
#include "Components/TextBlock.h"

namespace
{
    constexpr int32 MaxNicknameUiLen = 7;
}

void UChatMessageItem::SetMessage(const FString& PlayerName, const FString& Message)
{
    if(Text_PlayerName)
    {
        const FString ClampedPlayerName = PlayerName.Len() > MaxNicknameUiLen ? PlayerName.Left(MaxNicknameUiLen) : PlayerName;
        Text_PlayerName->SetText(FText::FromString(ClampedPlayerName));
    }
    if(Text_Message)
    {
        Text_Message->SetText(FText::FromString(Message));
    }
}
