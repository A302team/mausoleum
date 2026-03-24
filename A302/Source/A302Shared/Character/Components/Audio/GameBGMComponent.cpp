// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/Components/Audio/GameBGMComponent.h"
#include "Character/Components/Audio/MaliceBGMComponent.h" // Added
#include "Kismet/GameplayStatics.h"                        // Added
#include "Components/AudioComponent.h"                     // Added
#include "GameFramework/PlayerController.h"                // Added
#include "UObject/ConstructorHelpers.h"                    // Added

UGameBGMComponent::UGameBGMComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Added: 게임 기본 BGM 에셋 로드
	static ConstructorHelpers::FObjectFinder<USoundBase> GameAsset(TEXT("/Game/WorkSpace/Sound/BGM/GameBgm"));
	if (GameAsset.Succeeded())
	{
		GameBGM = GameAsset.Object;
	}
}

void UGameBGMComponent::BeginPlay()
{
	Super::BeginPlay();

	// Added: 로컬 플레이어 전용 체크
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	// Added: MaliceBGMComponent 참조 획득
	MaliceBGMComponentRef = PC->FindComponentByClass<UMaliceBGMComponent>();

	// Added: 게임 시작 시 기본 BGM 재생
	PlayGameBGM();
}

void UGameBGMComponent::HandleMaliceState(int32 MaliceCount)
{
	// Added: 로컬 플레이어 전용 체크
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	if (MaliceCount >= 3)
	{
		if (!bIsMaliceState)
		{
			// Added: GameBGM 정지
			StopGameBGM();
			
			// Added: MaliceBGMComponent를 통해 MaliceBGM 재생
			if (MaliceBGMComponentRef)
			{
				MaliceBGMComponentRef->HandleMaliceBGM(MaliceCount);
			}
			
			bIsMaliceState = true;
		}
	}
	else
	{
		if (bIsMaliceState)
		{
			// Added: MaliceBGMComponent를 통해 MaliceBGM 정지
			if (MaliceBGMComponentRef)
			{
				MaliceBGMComponentRef->HandleMaliceBGM(MaliceCount);
			}
			
			// Added: GameBGM 재개
			PlayGameBGM();
			
			bIsMaliceState = false;
		}
	}
}

void UGameBGMComponent::PlayGameBGM()
{
	// Added
	if (GameBGM)
	{
		GameBGMAudioComponent = UGameplayStatics::SpawnSound2D(this, GameBGM);
		if (GameBGMAudioComponent)
		{
			GameBGMAudioComponent->bAutoDestroy = false;
		}
	}
}

void UGameBGMComponent::StopGameBGM()
{
	// Added
	if (GameBGMAudioComponent)
	{
		GameBGMAudioComponent->Stop();
		GameBGMAudioComponent = nullptr;
	}
}
