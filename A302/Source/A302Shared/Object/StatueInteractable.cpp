// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/StatueInteractable.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Engine/GameInstance.h"
#include "Interface/A302ServerRewardBridge.h"
#include "GameData/RewardTypes.h"
#include "GameFramework/GameModeBase.h"
#include "NiagaraComponent.h"

AStatueInteractable::AStatueInteractable()
{
	CurrentInteractType = EInteractType::Hold;
	bReplicates = true;

	StatueEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("StatueEffectComponent"));
	StatueEffectComponent->SetupAttachment(Mesh);
}

void AStatueInteractable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AStatueInteractable, CurrentProgress);
	DOREPLIFETIME(AStatueInteractable, MaxProgress);
	DOREPLIFETIME(AStatueInteractable, bIsCompleted);
}

void AStatueInteractable::OnRep_IsCompleted()
{
	if (bIsCompleted)
	{
		if (StatueEffectComponent)
		{
			StatueEffectComponent->Deactivate();
		}
	}
}

void AStatueInteractable::OnServerHoldProgress(float DeltaTime, ACharacter* Interactor)
{
	if (!HasAuthority() || bIsCompleted) return;

	CurrentProgress += DeltaTime * ProgressSpeed;
	if (CurrentProgress >= MaxProgress)
	{
		CurrentProgress = MaxProgress;
		bIsCompleted = true;
		OnRep_IsCompleted(); // 서버(방장) 화면에서도 시각적 효과가 즉시 꺼지도록 수동 호출

		if (UWorld* World = GetWorld())
		{
			if (IA302ServerRewardBridge* ServerBridge = Cast<IA302ServerRewardBridge>(World->GetAuthGameMode()))
			{
				ServerBridge->NotifyInteractionRewardResolved(Interactor, GetRewardDefinition(), ERewardCategory::GroupEvent);
			}
		}
	}
}
