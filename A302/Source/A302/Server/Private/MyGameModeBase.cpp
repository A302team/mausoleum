// Fill out your copyright notice in the Description page of Project Settings.


#include "Server/Private/MyGameModeBase.h"
#include "MyGameModeBase.h"

AMyGameModeBase::AMyGameModeBase()
{
    static ConstructorHelpers::FClassFinder<APawn> DefaultPawnClassRef(TEXT("/Script/A302.MyCharacter"));
    // Default Pawn Class
    if(DefaultPawnClassRef.Class)
    {
        DefaultPawnClass = DefaultPawnClassRef.Class;
    }

    static ConstructorHelpers::FClassFinder<APlayerController> PlayerControllerClassRef(TEXT("/Script/A302.MyPlayerController"));
    if(PlayerControllerClassRef.Class)
    {
        PlayerControllerClass = PlayerControllerClassRef.Class;
    }
}