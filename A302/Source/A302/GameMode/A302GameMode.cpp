// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMode/A302GameMode.h"
#include "GameMode/A302GameState.h"
#include "GameMode/A302PlayerState.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "Manager/SpawnManager.h"
#include "Kismet/GameplayStatics.h"

AA302GameMode::AA302GameMode()
{
    DefaultPawnClass        = nullptr;
    PlayerControllerClass   = AMyPlayerController::StaticClass();
    GameStateClass          = AA302GameState::StaticClass();
    PlayerStateClass        = AA302PlayerState::StaticClass();
}

void AA302GameMode::BeginPlay()
{
    Super::BeginPlay();

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnManager::StaticClass(), FoundActors);

    if(FoundActors.Num() > 0)
    {
        SpawnManager = Cast<ASpawnManager>(FoundActors[0]);
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] Find SpawnManager."));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] Can't find SpawnManager"));
    }

    UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] BeginPlay"));
}

void AA302GameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    FInputModeGameOnly InputMode;
    NewPlayer->SetInputMode(InputMode);
    NewPlayer->bShowMouseCursor = false;

    UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] 플레이어 접속"));

    SpawnPlayer(NewPlayer);
}

void AA302GameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);
    UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] 플레이어 퇴장"));
}

void AA302GameMode::SpawnPlayer(APlayerController* PlayerController)
{
    if(!SpawnManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameMode] No SpawnManager."));
        return;
    }

    FTransform SpawnTransform = SpawnManager->GetRandomPlayerSpawnTransform(CurrentStage);

    AMyCharacter* Character = GetWorld()->SpawnActor<AMyCharacter>(
        AMyCharacter::StaticClass(),
        SpawnTransform
    );

    if(Character)
    {
        PlayerController->Possess(Character);
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameMode] Success SpawnPlayer."));
    }
}