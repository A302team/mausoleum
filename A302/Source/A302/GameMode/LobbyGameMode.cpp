// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyGameMode.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "UI/LobbyWidget.h"
#include "Blueprint/UserWidget.h"  


ALobbyGameMode::ALobbyGameMode()
{
    WebSocketManager = CreateDefaultSubobject<UWebSocketManager>(TEXT("WebSocketManager"));
}

void ALobbyGameMode::BeginPlay()
{
    Super::BeginPlay();

    WebSocketManager -> Connect(TEXT("ws://localhost:9001"));

    WebSocketManager -> OnMessageReceived.AddDynamic(this, &ALobbyGameMode::OnMessageReceived);

    if(LobbyWidgetClass)
    {
        LobbyWidget = CreateWidget<ULobbyWidget>(GetWorld(), LobbyWidgetClass);
        if(LobbyWidget)
        {
            LobbyWidget->AddToViewport();
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[GameMode/LobbyGameMode] Begin Play."))
}

void ALobbyGameMode::SendToServer(const FString& Message)
{
    if(WebSocketManager)
    {
        WebSocketManager -> SendMessage(Message);
    }
}

void ALobbyGameMode::OnMessageReceived(const FString& Message)
{
    UE_LOG(LogTemp, Log, TEXT("[GameMode/LobbyGameMode] ServerMessage : %s"), *Message);

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);
}