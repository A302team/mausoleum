#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BaseItem.generated.h"

class UItemInstance;
class UItemDefinition;
class ACharacter;

UCLASS(BlueprintType, Abstract)
class A302SHARED_API UBaseItem : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Item")
    void Initialize(UItemInstance* InInstance)
    {
        Instance = InInstance;
    }

    UFUNCTION(BlueprintCallable, Category="Item")
    UItemInstance* GetInstance() const { return Instance; }

    UFUNCTION(BlueprintCallable, Category="Item")
    const UItemDefinition* GetDefinition() const;
    
    virtual void OnItemAcquired(class AMyCharacter* OwnerCharacter) const {}
    virtual void OnItemUsed(class AMyCharacter* OwnerCharacter) const {}
    virtual bool ResolveServerTargetedUse(
        class AMyCharacter* OwnerCharacter,
        AActor* TargetActor,
        FString& OutSystemMessage
    ) const
    {
        return false;
    }

protected:
    UPROPERTY()
    TObjectPtr<UItemInstance> Instance;
};
