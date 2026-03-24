// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/StatueInteractable.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Engine/GameInstance.h"
#include "Interface/A302ServerRewardBridge.h"
#include "GameData/RewardTypes.h"
#include "GameFramework/GameModeBase.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

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

void AStatueInteractable::OnRep_CurrentProgress()
{
	if (bIsCompleted || !StatueEffectComponent || MaxProgress <= 0.0f)
	{
		return;
	}

	float Ratio = FMath::Clamp(CurrentProgress / MaxProgress, 0.0f, 1.0f);
	// 0%일 때 알파 1.0, 100%일 때 알파 0.5 (절반으로 옅어짐)
	float AlphaValue = FMath::Lerp(1.0f, 0.5f, Ratio);

	StatueEffectComponent->SetVariableFloat(AlphaParameterName, AlphaValue);
}

void AStatueInteractable::OnRep_IsCompleted()
{
	if (bIsCompleted)
	{
		if (StatueEffectComponent)
		{
			if (ExplosionEffectSystem)
			{
				StatueEffectComponent->SetAsset(ExplosionEffectSystem);
				StatueEffectComponent->SetVariableFloat(AlphaParameterName, 1.0f); // 폭발은 원래 밝기
				StatueEffectComponent->ReinitializeSystem();
			}
			else
			{
				StatueEffectComponent->Deactivate();
			}
		}
	}
}

void AStatueInteractable::OnServerHoldProgress(float DeltaTime, ACharacter* Interactor)
{
	if (!HasAuthority() || bIsCompleted) return;

	CurrentProgress += DeltaTime * ProgressSpeed;
	OnRep_CurrentProgress(); // 서버에서도 시각적인 알파 변경 즉시 반영
	
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
