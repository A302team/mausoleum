#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameData/RewardTypes.h"
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
    // 사용 가능한지 검사 (UI/입력 처리에서 먼저 체크 가능)
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Item|Use")
    bool CanUse(ACharacter* Instigator, const FItemTargetData& TargetData) const;

    // 실제 사용 실행
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Item|Use")
    bool Use(ACharacter* Instigator, const FItemTargetData& TargetData);
};
