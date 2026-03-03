#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BaseItem.generated.h"

class UItemInstance;
class UItemDefinition;

UCLASS(BlueprintType, Abstract)
class A302_API UBaseItem : public UObject
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

protected:
    UPROPERTY()
    TObjectPtr<UItemInstance> Instance;
};
