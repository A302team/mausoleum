// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/A302GameMode.h"
#include "GameMode/A302GameState.h"
#include "GameMode/A302PlayerState.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"

AA302GameMode::AA302GameMode()
{
    DefaultPawnClass        = AMyCharacter::StaticClass();
    PlayerControllerClass   = AMyPlayerController::StaticClass();
    GameStateClass          = AA302GameState::StaticClass();
    PlayerStateClass        = AA302PlayerState::StaticClass();
}

void AA302GameMode::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Log, TEXT("[A302GameMode] BeginPlay"));
}

void AA302GameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);
    UE_LOG(LogTemp, Log, TEXT("[A302GameMode] 플레이어 접속"));
}

void AA302GameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);
    UE_LOG(LogTemp, Log, TEXT("[A302GameMode] 플레이어 퇴장"));
}