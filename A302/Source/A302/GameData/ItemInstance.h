#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemInstance.generated.h"

class UItemDefinition;

UCLASS(BlueprintType)
class MYGAME_API UItemInstance : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Item")
    TObjectPtr<const UItemDefinition> Definition;

    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Item")
    int32 StackCount = 1;

    UFUNCTION(BlueprintCallable, Category="Item")
    void Init(const UItemDefinition* InDef, int32 InStackCount = 1)
    {
        Definition = InDef;
        StackCount = InStackCount;
    }

    UFUNCTION(BlueprintCallable, Category="Item")
    void Consume(int32 Amount = 1)
    {
        StackCount = FMath::Max(0, StackCount - Amount);
    }

    UFUNCTION(BlueprintCallable, Category="Item")
    bool IsEmpty() const
    {
        return StackCount <= 0;
    }
};