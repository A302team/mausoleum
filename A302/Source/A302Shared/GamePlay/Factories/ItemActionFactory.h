#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemActionFactory.generated.h"

class UItemInstance;
class UBaseItem;

UCLASS(BlueprintType)
class A302SHARED_API UItemActionFactory : public UObject
{
    GENERATED_BODY()

public:
    // Instance -> Definition.RewardLogicClass -> NewObject
    UFUNCTION(BlueprintCallable, Category="Item")
    UBaseItem* CreateLogic(UObject* Outer, UItemInstance* Instance) const;
};
