// Fill out your copyright notice in the Description page of Project Settings.

#include "LobbyGameMode.h"
#include "GameFramework/GameState.h"

ALobbyGameMode::ALobbyGameMode()
{
    GameStateClass = AGameState::StaticClass();
}

void ALobbyGameMode::BeginPlay()
{
    Super::BeginPlay();
}
