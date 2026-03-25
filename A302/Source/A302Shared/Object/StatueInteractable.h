// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Object/BaseInteractable.h"
#include "StatueInteractable.generated.h"

/**
 * 
 */
UCLASS()
class A302SHARED_API AStatueInteractable : public ABaseInteractable
{
	GENERATED_BODY()
	
public:
	AStatueInteractable();

	virtual void OnServerHoldProgress(float DeltaTime, class ACharacter* Interactor) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_CurrentProgress, Category = "Statue")
	float CurrentProgress = 0.0f;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Statue")
	void ForceComplete();

	UFUNCTION()
	void OnRep_CurrentProgress();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Statue")
	float MaxProgress = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_IsCompleted, Category = "Statue")
	bool bIsCompleted = false;

	UFUNCTION()
	void OnRep_IsCompleted();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ForcePlayEffectAndDisableCollision();

	// 1초 누를 때마다 이만큼 상승 (권장 20 = 5초짜리)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Statue")
	float ProgressSpeed = 20.0f;

	// 인스펙터에서 이펙트를 꽂을 수 있는 나이아가라 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Statue|Effect")
	class UNiagaraComponent* StatueEffectComponent;

	// 완주 완료 시 전환될 폭발 이펙트 나이아가라 시스템
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Statue|Effect")
	class UNiagaraSystem* ExplosionEffectSystem;

	// 나이아가라 이펙트의 투명도를 조절할 User 변수 이름 (예: "User.Alpha" 또는 "Alpha")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Statue|Effect")
	FName AlphaParameterName = TEXT("Alpha");

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
