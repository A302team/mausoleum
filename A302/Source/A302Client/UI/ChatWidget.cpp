// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ChatWidget.h"
#include "UI/ChatMessageItem.h"
#include "GameMode/A302GameInstance.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Network/LobbyConstants.h"

void UChatWidget::NativeConstruct()
{
    Super::NativeConstruct();

	GI = Cast<UA302GameInstance>(UGameplayStatics::GetGameInstance(this));

	if (Btn_Send)
	{
        Btn_Send->OnClicked.AddDynamic(this, &UChatWidget::OnSendClicked);
    }

    if (Input_ChatMessage)
    {
        Input_ChatMessage->OnTextCommitted.AddDynamic(this, &UChatWidget::OnInputCommitted);
    }

	if (GI)
	{
		GI->OnChatMessageReceived.AddDynamic(this, &UChatWidget::OnChatMessageReceived);
	}

	SetIsFocusable(true);
}

void UChatWidget::NativeDestruct()
{
    Super::NativeDestruct();

    if (GI)
    {
        GI->OnChatMessageReceived.RemoveDynamic(this, &UChatWidget::OnChatMessageReceived);
    }
}

FReply UChatWidget::NativeOnKeyDown(const FGeometry &InGeometry, const FKeyEvent &InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Enter)
    {
        Input_ChatMessage->SetKeyboardFocus();
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UChatWidget::OnSendClicked()
{
    SendMessage();
}

void UChatWidget::OnInputCommitted(const FText &text, ETextCommit::Type CommitMethod)
{
    if (CommitMethod == ETextCommit::OnEnter)
    {
        SendMessage();
    }
}

void UChatWidget::SendMessage()
{
	if (!GI)
		return;

	if (GI->CurrentRoomCode.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[UI/ChatWidget] Ignore chat send because CurrentRoomCode is empty."));
		return;
	}

    FString Message = Input_ChatMessage->GetText().ToString();
    if (Message.IsEmpty())
        return;

    TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);

    Data->SetStringField(LobbyProtocol::KeyMessage, Message);

	if (GI)
	{
		Data->SetStringField(LobbyProtocol::KeyRoomCode, GI->CurrentRoomCode);
		Data->SetStringField(LobbyProtocol::KeyPlayerName, GI->MyPlayerName);
	}

    TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
    Json->SetStringField(LobbyProtocol::KeyType, LobbyProtocol::ReqChatMessage);
    Json->SetObjectField(LobbyProtocol::KeyData, Data);

    FString Output;
    TSharedRef<TJsonWriter<>> writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Json.ToSharedRef(), writer);

	if (GI)
	{
		GI->SendToServer(Output);
	}

	Input_ChatMessage->SetText(FText::GetEmpty());

    UE_LOG(LogTemp, Log, TEXT("[UI/ChatWidget] 메세지 전송: %s"), *Message);

    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this]()
                                           {
        if (Input_ChatMessage) {
            Input_ChatMessage->SetKeyboardFocus();
        } }, 0.05f, false);
}

void UChatWidget::OnChatMessageReceived(const FString &PlayerName, const FString &Message)
{
    if (!GI || GI->CurrentRoomCode.IsEmpty())
    {
        return;
    }

    AddMessage(PlayerName, Message);
}

void UChatWidget::AddMessage(const FString &PlayerName, const FString &Message)
{
    if (!ChatMessageItemClass || !ScrollBox_Message)
        return;

    UChatMessageItem *Item = CreateWidget<UChatMessageItem>(GetWorld(), ChatMessageItemClass);
    if (Item)
    {
        Item->SetMessage(PlayerName, Message);
        ScrollBox_Message->AddChild(Item);

        ScrollBox_Message->ScrollToEnd();
    }
}

void UChatWidget::FocusChatInput()
{
    if (Input_ChatMessage)
    {
        Input_ChatMessage->SetKeyboardFocus();
    }
}
