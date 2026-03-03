#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "DummyCharacter.generated.h"

class UCombatStatusComponent;
class UItemDefinition;
class UItemActionFactory;
class UItemInstance;
class UBaseItem;

UCLASS()
class A302_API ADummyCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    ADummyCharacter();

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

    UPROPERTY(EditDefaultsOnly, Category="Item|Definition")
    TObjectPtr<UItemDefinition> KnifeDef;

    UPROPERTY(EditDefaultsOnly, Category="Item|Definition")
    TObjectPtr<UItemDefinition> ShieldDef;

    UPROPERTY(EditAnywhere, Category="Item|Test", meta=(ClampMin="0"))
    int32 InitialShieldStack = 1;

private:
    void SetupInitialShield();

    UPROPERTY()
    TObjectPtr<UItemActionFactory> ItemActionFactory;

    UPROPERTY()
    TObjectPtr<UItemInstance> ShieldInstance;

    UPROPERTY()
    TObjectPtr<UBaseItem> ShieldLogic;

    bool bIsDead = false;
};
