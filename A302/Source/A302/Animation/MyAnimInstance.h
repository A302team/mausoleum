#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "MyAnimInstance.generated.h"

class AMyCharacter;

UCLASS()
class A302_API UMyAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

public:

    UPROPERTY(BlueprintReadOnly, Category="Movement")
    float Speed = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="Movement")
    bool bIsInAir = false;

    UPROPERTY(BlueprintReadOnly, Category="Combat")
    bool bIsAttacking = false;

    UPROPERTY(BlueprintReadOnly, Category="Movement")
    float MoveDirection = 0.f;


private:

    UPROPERTY()
    AMyCharacter* CachedCharacter = nullptr;
};