// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Network/WebSocketManager.h"
#include "LobbyGameMode.generated.h"

UCLASS()
class A302_API ALobbyGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ALobbyGameMode();

    virtual void BeginPlay() override;

    // 위젯 클래스
    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<class ULobbyWidget> LobbyWidgetClass;

    UPROPERTY()
    TObjectPtr<class ULobbyWidget> LobbyWidget;

    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<class UWaitingRoomWidget> WaitingRoomWidgetClass;

    UPROPERTY()
    TObjectPtr<class UWaitingRoomWidget> WaitingRoomWidget;

    // 위젯 표시
    void ShowWaitingRoom(const FString& RoomCode);
};