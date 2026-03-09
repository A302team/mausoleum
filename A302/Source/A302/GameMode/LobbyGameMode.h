// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameMode/A302GameInstance.h" // For FRoomInfo
#include "LobbyGameMode.generated.h"

class ULobbyWidget;
class UWaitingRoomWidget;

UCLASS()
class A302_API ALobbyGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ALobbyGameMode();
    virtual void BeginPlay() override;

};