// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ChatMessageItem.h"
#include "Components/TextBlock.h"

void UChatMessageItem::SetMessage(const FString& PlayerName, const FString& Message)
{
    if(Text_PlayerName)
    {
        Text_PlayerName->SetText(FText::FromString(PlayerName));
    }
    if(Text_Message)
    {
        Text_Message->SetText(FText::FromString(Message));
    }
}