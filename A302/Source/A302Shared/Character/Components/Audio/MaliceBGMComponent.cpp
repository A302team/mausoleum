// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/Components/Audio/MaliceBGMComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "GameFramework/PlayerController.h" // Added

UMaliceBGMComponent::UMaliceBGMComponent()
{
	static ConstructorHelpers::FObjectFinder<USoundBase> BGMAsset(TEXT("/Game/WorkSpace/Sound/BGM/MaliceBgm"));
	if (BGMAsset.Succeeded())
	{
		MaliceBGM = BGMAsset.Object;
	}
}

void UMaliceBGMComponent::HandleMaliceBGM(int32 MaliceCount)
{
	// Added: Safety check - only run on local player
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}
	// End Added

	if (MaliceCount >= 3)
	{
		if (!bIsMaliceBGMPlaying && MaliceBGM)
		{
			BGMAudioComponent = UGameplayStatics::SpawnSound2D(this, MaliceBGM);
			if (BGMAudioComponent)
			{
				BGMAudioComponent->bAutoDestroy = false;
				bIsMaliceBGMPlaying = true;
			}
		}
	}
	else
	{
		if (bIsMaliceBGMPlaying && BGMAudioComponent)
		{
			BGMAudioComponent->Stop();
			bIsMaliceBGMPlaying = false;
		}
	}
}
