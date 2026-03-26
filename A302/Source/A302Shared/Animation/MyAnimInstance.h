#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Interface/A302AnimationBridge.h"
#include "MyAnimInstance.generated.h"

class ACharacter;
class UAnimMontage;
class UCombatStatusComponent;

UCLASS()
class A302SHARED_API UMyAnimInstance : public UAnimInstance, public IA302AnimationBridge
{
    GENERATED_BODY()

public:

    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;
    bool IsDead() const { return bIsDead; }

public:

    // Movement

    UPROPERTY(BlueprintReadOnly, Category="Movement")
    float Speed = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="Movement")
    bool bIsInAir = false;

    UPROPERTY(BlueprintReadOnly, Category="Movement")
    float MoveDirection = 0.f;


    // Combat State

    UPROPERTY(BlueprintReadOnly, Replicated, Category="Combat")
    bool bIsAttacking = false;

    UPROPERTY(BlueprintReadOnly, Replicated, Category="Combat")
    bool bIsBlocking = false;

    UPROPERTY(BlueprintReadOnly, Replicated, Category="Interaction")
    bool bIsInteracting = false;

    UPROPERTY(BlueprintReadOnly, Replicated, Category="Combat")
    bool bIsDead = false;


    // Montages

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat")
    UAnimMontage* AttackMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat")
    UAnimMontage* BlockMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction")
    UAnimMontage* InteractMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat")
    UAnimMontage* DeathMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat")
    UAnimMontage* TimeKnifeMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction")
    UAnimMontage* StatueInteractMontage;

    // Dance Montages – 블루프린트(BP_MyCharacter)의 AnimInstance에서 할당
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dance")
    UAnimMontage* DanceMontage1 = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dance")
    UAnimMontage* DanceMontage2 = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dance")
    UAnimMontage* DanceMontage3 = nullptr;


    // Gameplay → AnimInstance 호출용 함수

    UFUNCTION(BlueprintCallable)
    void PlayAttackMontage();

    UFUNCTION(BlueprintCallable)
    void PlayBlockMontage();

    UFUNCTION(BlueprintCallable)
    void PlayInteractMontage();

    UFUNCTION(BlueprintCallable)
    void PlayDeathMontage();

    UFUNCTION(BlueprintCallable)
    void PlayTimeKnifeMontage();

    UFUNCTION(BlueprintCallable)
    void PlayStatueInteractMontage();

    UFUNCTION(BlueprintCallable)
    void StopStatueInteractMontage();

    // Dance 몽타주 재생/중단 – DanceComponent 에서 호출
    UFUNCTION(BlueprintCallable)
    void PlayDanceMontage(int32 Index);

    UFUNCTION(BlueprintCallable)
    void StopDanceMontage();

    virtual void PlayAttackAnimation() override;
    virtual void PlayBlockAnimation() override;
    virtual void PlayInteractAnimation() override;
    virtual void PlayDeathAnimation() override;
    virtual void PlayTimeKnifeAnimation() override;
    virtual void PlayStatueInteractAnimation() override;
    virtual void StopStatueInteractAnimation() override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:

    UPROPERTY()
    ACharacter* CachedCharacter = nullptr;

    UPROPERTY()
    UCombatStatusComponent* CachedCombatComponent = nullptr;

    // 방어 애니메이션 재생 시, 이전 방어 횟수와 비교하여 방어 횟수가 증가했을 때만 애니메이션이 재생되도록 하기 위한 변수
    int32 PreviousShieldCount = 0;

    bool bAnimInitialized = false;
};
