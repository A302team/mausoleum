// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/WaitingRoomWidget.h"
#include "UI/PlayerListItem.h"
#include "GameMode/LobbyGameMode.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

void UWaitingRoomWidget::NativeConstruct()
{
    Super::NativeConstruct();

    LobbyGameMode = Cast<ALobbyGameMode>(UGameplayStatics::GetGameMode(this));

    if (Btn_Ready)
    {
        Btn_Ready->OnClicked.AddDynamic(this, &UWaitingRoomWidget::OnReadyClicked);

        if (LobbyGameMode && LobbyGameMode->bIsHost)
        {
            Btn_Ready->SetVisibility(ESlateVisibility::Hidden);
        }
    }
    if (Btn_StartGame)
    {
        Btn_StartGame->OnClicked.AddDynamic(this, &UWaitingRoomWidget::OnStartGameClicked);
        Btn_StartGame->SetVisibility(
            LobbyGameMode && LobbyGameMode->bIsHost
                ? ESlateVisibility::Visible
                : ESlateVisibility::Hidden);
    }
    if (Btn_Leave)
    {
        Btn_Leave->OnClicked.AddDynamic(this, &UWaitingRoomWidget::OnLeaveClicked);
    }

    if (LobbyGameMode)
    {
        LobbyGameMode->OnPlayerEntered.AddDynamic(this, &UWaitingRoomWidget::OnPlayerEntered);
        LobbyGameMode->OnPlayerReady.AddDynamic(this, &UWaitingRoomWidget::OnPlayerReady);
        LobbyGameMode->OnPlayerLeft.AddDynamic(this, &UWaitingRoomWidget::OnPlayerLeft);
    }
}

void UWaitingRoomWidget::NativeDestruct()
{
    Super::NativeDestruct();

    if (LobbyGameMode)
    {
        LobbyGameMode->OnPlayerEntered.RemoveDynamic(this, &UWaitingRoomWidget::OnPlayerEntered);
        LobbyGameMode->OnPlayerReady.RemoveDynamic(this, &UWaitingRoomWidget::OnPlayerReady);
        LobbyGameMode->OnPlayerLeft.RemoveDynamic(this, &UWaitingRoomWidget::OnPlayerLeft);
    }
}

void UWaitingRoomWidget::SetRoomCode(const FString &RoomCode)
{
    if (Text_RoomCode)
    {
        Text_RoomCode->SetText(FText::FromString(RoomCode));
    }
}

void UWaitingRoomWidget::OnPlayerEntered(const FString &PlayerName)
{
    if (!PlayerListItemClass || !ScrollBox_Players)
        return;

    UPlayerListItem *Item = CreateWidget<UPlayerListItem>(GetWorld(), PlayerListItemClass);
    if (Item)
    {
        Item->SetPlayerName(PlayerName);
        ScrollBox_Players->AddChild(Item);
        PlayerItems.Add(PlayerName, Item);
        UE_LOG(LogTemp, Log, TEXT("[UI/WaitingRoom] 플레이어 입장: %s"), *PlayerName);
    }
}

void UWaitingRoomWidget::OnPlayerReady(const FString &PlayerName)
{
    if (PlayerItems.Contains(PlayerName))
    {
        PlayerItems[PlayerName]->SetReady(true);
        UE_LOG(LogTemp, Log, TEXT("[UI/WaitingRoom] 플레이어 레디: %s"), *PlayerName);
    }
}

void UWaitingRoomWidget::OnPlayerLeft(const FString &PlayerName)
{
    if (PlayerItems.Contains(PlayerName))
    {
        PlayerItems[PlayerName]->RemoveFromParent();
        PlayerItems.Remove(PlayerName);
        UE_LOG(LogTemp, Log, TEXT("[UI/WaitingRoom] 플레이어 퇴장: %s"), *PlayerName);
    }
}

void UWaitingRoomWidget::OnReadyClicked()
{
    if (!LobbyGameMode)
        return;

    TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
    Data->SetStringField(TEXT("roomCode"), LobbyGameMode->CurrentRoomCode);
    Data->SetStringField(TEXT("playerName"), LobbyGameMode->MyPlayerName);

    TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
    Json->SetStringField(TEXT("type"), TEXT("ready"));
    Json->SetObjectField(TEXT("data"), Data);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

    LobbyGameMode->SendToServer(Output);
    UE_LOG(LogTemp, Log, TEXT("[UI/WaitingRoom] 레디!"));
}

void UWaitingRoomWidget::OnStartGameClicked()
{
    if (!LobbyGameMode)
        return;

    TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
    Data->SetStringField(TEXT("roomCode"), LobbyGameMode->CurrentRoomCode);
    Data->SetStringField(TEXT("playerName"), LobbyGameMode->MyPlayerName);

    TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
    Json->SetStringField(TEXT("type"), TEXT("start_game"));
    Json->SetObjectField(TEXT("data"), Data);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

    LobbyGameMode->SendToServer(Output);
    UE_LOG(LogTemp, Log, TEXT("[UI/WaitingRoom] 게임 시작 요청!"));
}

void UWaitingRoomWidget::OnLeaveClicked()
{
    if (!LobbyGameMode)
        return;

    UE_LOG(LogTemp, Log, TEXT("[UI/WaitingRoom] RoomCode: %s, PlayerName: %s"), 
        *LobbyGameMode->CurrentRoomCode, 
        *LobbyGameMode->MyPlayerName);

    TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
    Data->SetStringField(TEXT("roomCode"), LobbyGameMode->CurrentRoomCode);
    Data->SetStringField(TEXT("playerName"), LobbyGameMode->MyPlayerName);

    TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
    Json->SetStringField(TEXT("type"), TEXT("leave_room"));
    Json->SetObjectField(TEXT("data"), Data);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

    LobbyGameMode->SendToServer(Output);

    // 닉네임 초기화
    LobbyGameMode->MyPlayerName = TEXT("");
    LobbyGameMode->CurrentRoomCode = TEXT("");
    LobbyGameMode->bIsHost = false;

    RemoveFromParent();
    UE_LOG(LogTemp, Log, TEXT("[UI/WaitingRoom] 방 나가기"));
}
