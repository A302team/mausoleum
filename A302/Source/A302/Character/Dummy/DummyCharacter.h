#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "DummyCharacter.generated.h"

class UCombatStatusComponent;
class UItemDefinition;
class UMaliceComponent;

UCLASS()
class A302_API ADummyCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ADummyCharacter();
    int32 GetCurrentMaliceCount() const;

    virtual float TakeDamage(
        float DamageAmount,
        struct FDamageEvent const& DamageEvent,
        class AController* EventInstigator,
        AActor* DamageCauser
    ) override;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat")
    TObjectPtr<UCombatStatusComponent> CombatStatusComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Malice")
    TObjectPtr<UMaliceComponent> MaliceComponent;

    UPROPERTY(EditDefaultsOnly, Category="Item|Definition")
    TObjectPtr<UItemDefinition> KnifeDef;

    UPROPERTY(EditDefaultsOnly, Category="Item|Definition")
    TObjectPtr<UItemDefinition> ShieldDef;

    UPROPERTY(EditAnywhere, Category="Item|Test", meta=(ClampMin="0"))
    int32 InitialShieldStack = 1;

    UPROPERTY(EditAnywhere, Category="Combat|Test")
    bool bEnableAutoAttack = true;

    UPROPERTY(EditAnywhere, Category="Combat|Test", meta=(ClampMin="0.1"))
    float AutoAttackInterval = 3.0f;

    UPROPERTY(EditAnywhere, Category="Combat|Test", meta=(ClampMin="0.0"))
    float AutoAttackRange = 300.0f;

    UPROPERTY(EditAnywhere, Category="Combat|Test", meta=(ClampMin="0.0"))
    float AutoAttackDamage = 9999.0f;

    UPROPERTY(EditAnywhere, Category="Malice|Test", meta=(ClampMin="0"))
    int32 InitialMaliceCount = 3;

private:
    void SetupInitialShield();
    void SetupInitialMalice();
    void TryAutoAttackPlayer();

    bool bIsDead = false;
    FTimerHandle AutoAttackTimerHandle;
};
