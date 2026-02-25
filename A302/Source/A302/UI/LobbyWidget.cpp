// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/LobbyWidget.h"
#include "GameMode/LobbyGameMode.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

void ULobbyWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Get GameMode
    LobbyGameMode = Cast<ALobbyGameMode>(UGameplayStatics::GetGameMode(this));

    if(Btn_CreateRoom)
    {
        Btn_CreateRoom -> OnClicked.AddDynamic(this, &ULobbyWidget::OnCreateRoomClicked);
    }

    if(Btn_Ready)
    {
        Btn_Ready -> OnClicked.AddDynamic(this, &ULobbyWidget::OnReadyClicked);
    }

    if(Btn_StartGame)
    {
        Btn_StartGame -> OnClicked.AddDynamic(this, &ULobbyWidget::OnStartGameClicked);
    }
}

void ULobbyWidget::OnCreateRoomClicked()
{
    FString RoomId = Input_RoomId->GetText().ToString();
    FString PlayerName = Input_PlayerName->GetText().ToString();

    // make Json
    TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
    Data->SetStringField(TEXT("roomId"), RoomId);
    Data->SetStringField(TEXT("playerId"), FGuid::NewGuid().ToString());
    Data->SetStringField(TEXT("playerName"), PlayerName);

    TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
    Json->SetStringField(TEXT("type"), TEXT("join_room"));
    Json->SetObjectField(TEXT("data"), Data);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

    LobbyGameMode->SendToServer(Output);
    UE_LOG(LogTemp, Log, TEXT("[UI/LobbyWidget] Enter the Room : %s"), *Output);
}

void ULobbyWidget::OnReadyClicked()
{
    UE_LOG(LogTemp, Log, TEXT("[UI/LobbyWidget] Ready Clicked!"));
}


void ULobbyWidget::OnStartGameClicked()
{
    UE_LOG(LogTemp, Log, TEXT("[UI/LobbyWidget] StartGame Clicked!"));

}