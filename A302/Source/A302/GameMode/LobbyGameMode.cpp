// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyGameMode.h"
#include "UI/LobbyWidget.h"
#include "UI/WaitingRoomWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameMode/A302GameInstance.h"
#include "Kismet/GameplayStatics.h"

ALobbyGameMode::ALobbyGameMode()
{
}

void ALobbyGameMode::BeginPlay()
{
    Super::BeginPlay();

    // 서버에서는 위젯 생성 안함
    if (GetNetMode() == NM_DedicatedServer) return;

    // 마우스 활성화
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (PC)
    {
        PC->bShowMouseCursor = true;
        PC->bEnableClickEvents = true;
        PC->SetInputMode(FInputModeUIOnly());
    }

    if (LobbyWidgetClass)
    {
        LobbyWidget = CreateWidget<ULobbyWidget>(GetWorld(), LobbyWidgetClass);
        if (LobbyWidget)
        {
            LobbyWidget->AddToViewport();
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[GameMode/LobbyGameMode] BeginPlay"));
}

void ALobbyGameMode::ShowWaitingRoom(const FString& RoomCode)
{
    if (WaitingRoomWidgetClass)
    {
        WaitingRoomWidget = CreateWidget<UWaitingRoomWidget>(GetWorld(), WaitingRoomWidgetClass);
        if (WaitingRoomWidget)
        {
            WaitingRoomWidget->SetRoomCode(RoomCode);
            WaitingRoomWidget->AddToViewport();

            UA302GameInstance* GI = Cast<UA302GameInstance>(GetGameInstance());
            if (GI)
            {
                WaitingRoomWidget->OnPlayerEntered(GI->MyPlayerName);
            }

            if (LobbyWidget)
            {
                LobbyWidget->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }
}