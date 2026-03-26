// Fill out your copyright notice in the Description page of Project Settings.


#include "Object/StatueInteractable.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Engine/GameInstance.h"
#include "Interface/A302ServerRewardBridge.h"
#include "Interface/A302ServerRewardSignals.h"
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
		Multicast_ForcePlayEffectAndDisableCollision_Implementation();
	}
}

void AStatueInteractable::Multicast_ForcePlayEffectAndDisableCollision_Implementation()
{
	if (StatueEffectComponent)
	{
		if (ExplosionEffectSystem)
		{
			StatueEffectComponent->SetAsset(ExplosionEffectSystem);
			StatueEffectComponent->SetVariableFloat(AlphaParameterName, 1.0f); // 폭발은 원래 밝기
			// 루프 해제는 Niagara 에셋에서 Loop Behavior → "Once"로 설정 필요
			StatueEffectComponent->ReinitializeSystem();
		}
		else
		{
			StatueEffectComponent->Deactivate();
		}
	}

	// 100% 완료된 석상은 더 이상 상호작용 불가능하도록 모든 컴포넌트의 레이캐스트 충돌을 무시합니다.
	TArray<UPrimitiveComponent*> Prims;
	GetComponents<UPrimitiveComponent>(Prims);
	for (UPrimitiveComponent* Prim : Prims)
	{
		Prim->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
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
			else if (Interactor && Interactor->HasAuthority() && GetRewardDefinition())
			{
				A302GetServerRewardResolvedSignal().Broadcast(Interactor, GetRewardDefinition(), ERewardCategory::GroupEvent);
			}
		}
	}
}

void AStatueInteractable::ForceComplete()
{
	if (!HasAuthority() || bIsCompleted) return;

	CurrentProgress = MaxProgress;
	bIsCompleted = true;
	OnRep_CurrentProgress();
	
	// 로컬 OnRep 호출 대신, 클라이언트들에게 확실하게 뿌리기 위해 NetMulticast 호출
	Multicast_ForcePlayEffectAndDisableCollision();

	// Note: We don't trigger the group event reward again to avoid multiple Phase advancement calls,
	// since this is just forcing the visualization and state for the remaining statues.
}
