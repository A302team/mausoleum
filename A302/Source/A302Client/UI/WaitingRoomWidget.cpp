// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/WaitingRoomWidget.h"
#include "UI/PlayerListItem.h"
#include "UI/ChatWidget.h"
#include "UI/LobbyWidget.h"
#include "GameMode/A302GameInstance.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

FReply UWaitingRoomWidget::NativeOnKeyDown(const FGeometry &InGeometry, const FKeyEvent &InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::Enter)
    {
        if (ChatWidget)
        {
            ChatWidget->FocusChatInput();
            return FReply::Handled();
        }
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UWaitingRoomWidget::NativeConstruct()
{
    Super::NativeConstruct();

    GI = Cast<UA302GameInstance>(UGameplayStatics::GetGameInstance(this));

    if (Btn_Ready)
    {
        Btn_Ready->OnClicked.AddDynamic(this, &UWaitingRoomWidget::OnReadyClicked);

        if (GI && GI->bIsHost)
        {
            Btn_Ready->SetVisibility(ESlateVisibility::Hidden);
        }
    }
    if (Btn_StartGame)
    {
        Btn_StartGame->OnClicked.AddDynamic(this, &UWaitingRoomWidget::OnStartGameClicked);
        Btn_StartGame->SetVisibility(
            GI && GI->bIsHost
                ? ESlateVisibility::Visible
                : ESlateVisibility::Hidden);
    }
    if (Btn_Leave)
    {
        Btn_Leave->OnClicked.AddDynamic(this, &UWaitingRoomWidget::OnLeaveClicked);
    }

    if (GI)
    {
        GI->OnPlayerEntered.AddDynamic(this, &UWaitingRoomWidget::OnPlayerEntered);
        GI->OnPlayerReady.AddDynamic(this, &UWaitingRoomWidget::OnPlayerReady);
        GI->OnPlayerLeft.AddDynamic(this, &UWaitingRoomWidget::OnPlayerLeft);
        GI->OnGameStarted.AddDynamic(this, &UWaitingRoomWidget::OnGameStarted);
    }
}

void UWaitingRoomWidget::NativeDestruct()
{
    Super::NativeDestruct();

    if (GI)
    {
        GI->OnPlayerEntered.RemoveDynamic(this, &UWaitingRoomWidget::OnPlayerEntered);
        GI->OnPlayerReady.RemoveDynamic(this, &UWaitingRoomWidget::OnPlayerReady);
        GI->OnPlayerLeft.RemoveDynamic(this, &UWaitingRoomWidget::OnPlayerLeft);
        GI->OnGameStarted.RemoveDynamic(this, &UWaitingRoomWidget::OnGameStarted);
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
    if (!GI)
        return;

    TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
    Data->SetStringField(TEXT("roomCode"), GI->CurrentRoomCode);
    Data->SetStringField(TEXT("playerName"), GI->MyPlayerName);

    TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
    Json->SetStringField(TEXT("type"), TEXT("ready"));
    Json->SetObjectField(TEXT("data"), Data);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

    GI->SendToServer(Output);
    UE_LOG(LogTemp, Log, TEXT("[UI/WaitingRoom] 레디!"));
}

void UWaitingRoomWidget::OnStartGameClicked()
{
    if (!GI)
        return;

    TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
    Data->SetStringField(TEXT("roomCode"), GI->CurrentRoomCode);
    Data->SetStringField(TEXT("playerName"), GI->MyPlayerName);

    TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
    Json->SetStringField(TEXT("type"), TEXT("start_game"));
    Json->SetObjectField(TEXT("data"), Data);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

    GI->SendToServer(Output);
    UE_LOG(LogTemp, Log, TEXT("[UI/WaitingRoom] 게임 시작 요청!"));
}

void UWaitingRoomWidget::OnLeaveClicked()
{
    if (!GI)
        return;

    UE_LOG(LogTemp, Log, TEXT("[UI/WaitingRoom] RoomCode: %s, PlayerName: %s"),
           *GI->CurrentRoomCode,
           *GI->MyPlayerName);

    TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
    Data->SetStringField(TEXT("roomCode"), GI->CurrentRoomCode);
    Data->SetStringField(TEXT("playerName"), GI->MyPlayerName);

    TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
    Json->SetStringField(TEXT("type"), TEXT("leave_room"));
    Json->SetObjectField(TEXT("data"), Data);

    FString Output;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
    FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

    GI->SendToServer(Output);

    // 닉네임 초기화
    GI->MyPlayerName = TEXT("");
    GI->CurrentRoomCode = TEXT("");
    GI->bIsHost = false;

    if (GI && GI->LobbyWidget)
    {
        GI->LobbyWidget->SetVisibility(ESlateVisibility::Visible);
    }

    RemoveFromParent();
    UE_LOG(LogTemp, Log, TEXT("[UI/WaitingRoom] 방 나가기"));
}

void UWaitingRoomWidget::OnGameStarted()
{
    if (GI && GI->WaitingRoomWidget == this)
    {
        GI->WaitingRoomWidget = nullptr;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC && GI)
    {
        UWorld* World = GI->GetWorld();
        if (World)
        {
            PC = UGameplayStatics::GetPlayerController(World, 0);
        }
    }

    if (PC)
    {
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = false;
        PC->bEnableClickEvents = false;
    }

    RemoveFromParent();
    UE_LOG(LogTemp, Log, TEXT("[UI/WaitingRoom] Game started. Waiting room UI closed."));
}

void UWaitingRoomWidget::OnHostChanged()
{
    // 방장으로 승격되었다면 레디 버튼을 감추고 게임 시작 버튼을 켭니다.
    if (Btn_Ready)
    {
        Btn_Ready->SetVisibility(ESlateVisibility::Hidden);
    }
    if (Btn_StartGame)
    {
        Btn_StartGame->SetVisibility(ESlateVisibility::Visible);
    }
}