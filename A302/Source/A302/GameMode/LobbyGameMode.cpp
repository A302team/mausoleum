// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyGameMode.h"
#include "UI/LobbyWidget.h"
#include "UI/WaitingRoomWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameMode/A302GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Network/GameNetworkSubsystem.h"

ALobbyGameMode::ALobbyGameMode()
{
}

void ALobbyGameMode::BeginPlay()
{
    Super::BeginPlay();
}
