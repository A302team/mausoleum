#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MyAnimInstance.generated.h"

class AMyCharacter;
class UCombatStatusComponent;
class UAnimMontage;

UCLASS()
class A302_API UMyAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

public:

    // Movement
    UPROPERTY(BlueprintReadOnly, Category="Movement")
    float Speed = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="Movement")
    bool bIsInAir = false;
    
    UPROPERTY(BlueprintReadOnly, Category="Movement")
    float MoveDirection = 0.f;

    // Combat
    UPROPERTY(BlueprintReadOnly, Category="Combat")
    bool bIsAttacking = false;

    UPROPERTY(BlueprintReadOnly, Category="Combat")
    bool bIsBlocking = false;

    // Block Animations
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat")
    UAnimMontage* BlockMontage;

    UFUNCTION(BlueprintCallable)
    void PlayBlockMontage();

private:

    UPROPERTY()
    AMyCharacter* CachedCharacter = nullptr;

    UPROPERTY()
    UCombatStatusComponent* CachedCombatComponent = nullptr;

    int32 PreviousShieldCount = 0;
};