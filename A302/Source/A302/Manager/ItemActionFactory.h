#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemActionFactory.generated.h"

class UItemInstance;
class UBaseItem;

UCLASS(BlueprintType)
class A302_API UItemActionFactory : public UObject
{
    GENERATED_BODY()

public:
    // Instance -> Definition.ItemLogicClass -> NewObject 로 생성
    UFUNCTION(BlueprintCallable, Category="Item")
    UBaseItem* CreateLogic(UObject* Outer, UItemInstance* Instance) const;
};
