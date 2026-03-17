#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CombatStatusComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShieldChanged, int32, NewCount);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302SHARED_API UCombatStatusComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCombatStatusComponent();

    UPROPERTY(BlueprintReadOnly, Category="Combat")
    int32 ShieldBlockCount = 0;

    UPROPERTY(BlueprintAssignable, Category="Combat")
    FOnShieldChanged OnShieldChanged;

    UFUNCTION(BlueprintCallable, Category="Combat")
    void AddShield(int32 Count);

    UFUNCTION(BlueprintCallable, Category="Combat")
    bool TryConsumeShieldToBlock();
};
