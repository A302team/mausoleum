#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameData/Items/ItemTypes.h"
#include "UsableItem.generated.h"

class ACharacter;

UINTERFACE(BlueprintType)
class UUsableItem : public UInterface
{
    GENERATED_BODY()
};

class IUsableItem
{
    GENERATED_BODY()

public:
    // ?�용 가?�한지 검??(UI/?�력 처리?�서 먼�? 체크 가??
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Item|Use")
    bool CanUse(ACharacter* Instigator, const FItemTargetData& TargetData) const;

    // ?�제 ?�용 ?�행
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Item|Use")
    bool Use(ACharacter* Instigator, const FItemTargetData& TargetData);
};

