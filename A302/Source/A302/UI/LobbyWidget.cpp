// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/LobbyWidget.h"
#include "UI/EnterRoomPopup.h"
#include "GameMode/LobbyGameMode.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

void ULobbyWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // Get GameMode
    LobbyGameMode = Cast<ALobbyGameMode>(UGameplayStatics::GetGameMode(this));

    if (Btn_CreateRoom)
    {
        Btn_CreateRoom->OnClicked.AddDynamic(this, &ULobbyWidget::OnCreateRoomClicked);
    }

    if (Btn_EnterRoom)
    {
        Btn_EnterRoom->OnClicked.AddDynamic(this, &ULobbyWidget::OnEnterRoomClicked);
    }

    if(Btn_FindRoom)
    {
        Btn_FindRoom->OnClicked.AddDynamic(this, &ULobbyWidget::OnFindRoomClicked);
    }

    if (Btn_Exit)
    {
        Btn_Exit->OnClicked.AddDynamic(this, &ULobbyWidget::OnExitClicked);
    }

    if (LobbyGameMode)
    {
        LobbyGameMode->OnRoomCreated.AddDynamic(this, &ULobbyWidget::OnRoomCreated);
    }
}

void ULobbyWidget::OnCreateRoomClicked()
{
    FString PlayerName = Input_PlayerName->GetText().ToString();

    if (PlayerName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[UI/LobbyWidget] Insert the Nickname"));
        return;
    }

    LobbyGameMode->MyPlayerName = PlayerName;

    // make Json
    TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
    Data->SetStringField(TEXT("playerName"), PlayerName);

    TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
    Json->SetStringField(TEXT("type"), TEXT("create_room"));
    Json->SetObjectField(TEXT("data"), Data);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

    LobbyGameMode->SendToServer(Output);
    UE_LOG(LogTemp, Log, TEXT("[UI/LobbyWidget] Enter the Room : %s"), *Output);
}

void ULobbyWidget::OnEnterRoomClicked()
{
    FString PlayerName = Input_PlayerName->GetText().ToString();
    FString RoomCode = Input_RoomCode->GetText().ToString();

    if(PlayerName.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[UI/LobbyWidget] Please enter the Nickname"));
        return;
    }

    if(RoomCode.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[UI/LobbyWidget] Insert the Room Code"));
        return;
    }

    LobbyGameMode->MyPlayerName = PlayerName;

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
    UE_LOG(LogTemp, Log, TEXT("[UI/LobbyWidget] Try Enter the room - Code: %s | Nickname: %s"), *RoomCode, *PlayerName);
}

void ULobbyWidget::OnFindRoomClicked()
{
    if (RoomListPopupClass) {
        URoomListPopup* Popup = CreateWidget<URoomListPopup>(GetWorld(), RoomListPopupClass);
        if (Popup) {
            Popup->AddToViewport();
            UE_LOG(LogTemp, Log, TEXT("[UI/LobbyWidget] Open room list popup"));
        }
    }
}

void ULobbyWidget::OnExitClicked()
{
    UE_LOG(LogTemp, Log, TEXT("[UI/LobbyWidget] Exit"));
    UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}

void ULobbyWidget::OnRoomCreated(const FString& RoomCode)
{
    if(Input_RoomCode)
    {
        Input_RoomCode->SetText(FText::FromString(RoomCode));
        UE_LOG(LogTemp, Log, TEXT("[UI/LobbyWidget] Room Code : %s"), *RoomCode);
    }
}