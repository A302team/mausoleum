// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ChatMessageItem.h"
#include "UI/NicknameUiConstants.h"
#include "Components/TextBlock.h"

void UChatMessageItem::SetMessage(const FString& PlayerName, const FString& Message)
{
    if(Text_PlayerName)
    {
        const FString ClampedPlayerName = PlayerName.Len() > A302UI::MaxNicknameUiLen ? PlayerName.Left(A302UI::MaxNicknameUiLen) : PlayerName;
        Text_PlayerName->SetText(FText::FromString(ClampedPlayerName));
    }
    if(Text_Message)
    {
        Text_Message->SetText(FText::FromString(Message));
    }
}
