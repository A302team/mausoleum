// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/EnterRoomPopup.h"
#include "GameMode/LobbyGameMode.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

void UEnterRoomPopup::NativeConstruct()
{
    Super::NativeConstruct();

    LobbyGameMode = Cast<ALobbyGameMode>(UGameplayStatics::GetGameMode(this));

    if(Btn_Confirm)
    {
        Btn_Confirm->OnClicked.AddDynamic(this, &UEnterRoomPopup::OnConfirmClicked);
    }
    if(Btn_Cancel)
    {
        Btn_Cancel->OnClicked.AddDynamic(this, &UEnterRoomPopup::OnCancelClicked);
    }
}

void UEnterRoomPopup::OnConfirmClicked()
{
    FString RoomCode = Input_RoomCode->GetText().ToString();
    
    if (RoomCode.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[UI/EnterRoomPopup] Please Enter the RoomCode." ));
        return;
    }

    if(!LobbyGameMode) return;
    FString PlayerName = LobbyGameMode->MyPlayerName;

    if (PlayerName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[UI/EnterRoomPopup] No Nickname. You Must Check The Code (+LobbyWidget.cpp Code)" ));
        return;
    }

    TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
    Data->SetStringField(TEXT("roomCode"), RoomCode);
    Data->SetStringField(TEXT("playerName"), PlayerName);

    TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
    Json->SetStringField(TEXT("type"), TEXT("join_room"));
    Json->SetObjectField(TEXT("data"), Data);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

    LobbyGameMode->SendToServer(Output);
    UE_LOG(LogTemp, Log, TEXT("[UI/EnterRoomPopup] Try Enter The room - Code : %s | Nickname : %s"), *RoomCode, *PlayerName);

    RemoveFromParent();
}

void UEnterRoomPopup::OnCancelClicked()
{
    UE_LOG(LogTemp, Log, TEXT("[UI/EnterRoomPopup] Close the Popup."));
    RemoveFromParent();
}
