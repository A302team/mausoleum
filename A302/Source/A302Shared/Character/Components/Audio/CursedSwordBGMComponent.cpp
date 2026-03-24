// Fill out your copyright notice in the Description page of Project Settings.

// Added: 저주받은 검 BGM 최우선 관리 컴포넌트

#include "Character/Components/Audio/CursedSwordBGMComponent.h"
#include "Character/Components/Audio/GameBGMComponent.h"
#include "Character/Components/Audio/MaliceBGMComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

UCursedSwordBGMComponent::UCursedSwordBGMComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// Added: 저주받은 검 BGM 에셋 로드
	static ConstructorHelpers::FObjectFinder<USoundBase> Asset(TEXT("/Game/WorkSpace/Sound/BGM/CursedSwordBgm"));
	if (Asset.Succeeded())
	{
		CursedSwordBGM = Asset.Object;
	}
}

void UCursedSwordBGMComponent::BeginPlay()
{
	Super::BeginPlay();

	// Added: 로컬 플레이어 전용 — 서버·다른 클라이언트는 즉시 반환
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}
}

// Added: 저주받은 검 획득 — 최우선 BGM으로 전환
void UCursedSwordBGMComponent::OnCursedSwordAcquired()
{
	// Added: 로컬 플레이어 안전 체크
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	bHasCursedSword = true;

	// Added: 현재 재생 중인 모든 BGM 정지 후 CursedSwordBGM 재생
	StopCurrentBGM();

	if (CursedSwordBGM)
	{
		AudioComponent = UGameplayStatics::SpawnSound2D(this, CursedSwordBGM);
		if (AudioComponent)
		{
			AudioComponent->bAutoDestroy = false;
		}
	}
}

// Added: 저주받은 검 사용 — 이전 BGM 상태로 복귀
void UCursedSwordBGMComponent::OnCursedSwordUsed()
{
	// Added: 로컬 플레이어 안전 체크
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	bHasCursedSword = false;

	// Added: CursedSwordBGM 정지 후 Malice 상태에 따라 복귀
	StopCurrentBGM();

	if (CurrentMaliceCount >= 3)
	{
		TriggerMaliceBGM(); // Added: MaliceBGM으로 복귀
	}
	else
	{
		TriggerGameBGM(); // Added: GameBGM으로 복귀
	}
}

// Added: Malice 수치 변경 처리 — 검 보유 중이면 무시하여 CursedSwordBGM 유지
void UCursedSwordBGMComponent::HandleMaliceState(int32 MaliceCount)
{
	CurrentMaliceCount = MaliceCount; // Added: 상태 기록 (OnCursedSwordUsed 복귀 판단용)

	// Added: 저주받은 검 보유 시 MaliceBGM 전환 무시 — CursedSwordBGM이 최우선
	if (bHasCursedSword)
	{
		return;
	}

	if (MaliceCount >= 3)
	{
		TriggerMaliceBGM(); // Added: 검 미보유 + Malice >= 3 → MaliceBGM
	}
}

// Added: 현재 재생 중인 모든 BGM 정지 (AudioComponent, GameBGM, MaliceBGM)
void UCursedSwordBGMComponent::StopCurrentBGM()
{
	// Added: 자체 CursedSword AudioComponent 정지
	if (AudioComponent)
	{
		AudioComponent->Stop();
		AudioComponent = nullptr;
	}

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		return;
	}

	// Added: GameBGMComponent 정지
	if (UGameBGMComponent* GameBGM = PC->FindComponentByClass<UGameBGMComponent>())
	{
		GameBGM->StopGameBGM();
	}

	// Added: MaliceBGMComponent 정지 (MaliceCount 0 전달로 정지 트리거)
	if (UMaliceBGMComponent* MaliceBGM = PC->FindComponentByClass<UMaliceBGMComponent>())
	{
		MaliceBGM->HandleMaliceBGM(0);
	}
}

// Added: 기존 MaliceBGMComponent를 통해 MaliceBGM 재생
void UCursedSwordBGMComponent::TriggerMaliceBGM()
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	if (UMaliceBGMComponent* MaliceBGM = PC->FindComponentByClass<UMaliceBGMComponent>())
	{
		MaliceBGM->HandleMaliceBGM(3); // Added: 3 이상 전달 → MaliceBGM 재생
	}
}

// Added: 기존 GameBGMComponent를 통해 GameBGM 재생
void UCursedSwordBGMComponent::TriggerGameBGM()
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	if (UGameBGMComponent* GameBGM = PC->FindComponentByClass<UGameBGMComponent>())
	{
		GameBGM->PlayGameBGM(); // Added: 기존 GameBGMComponent의 재생 함수 호출
	}
}
