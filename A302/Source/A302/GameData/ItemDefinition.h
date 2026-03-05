#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameData/ItemTypes.h"
#include "ItemDefinition.generated.h"

class UBaseItem;

UCLASS(BlueprintType)
class A302_API UItemDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
    FName ItemId;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
    TObjectPtr<UTexture2D> Icon;

    // 칼: Targeted / 방패: SelfCast
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Use")
    EItemUseMode UseMode = EItemUseMode::SelfCast;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Use")
    bool AutoUse = false;

    // 핵심: 이 Definition이 어떤 “로직 클래스”로 Use를 수행하는가
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Logic")
    TSubclassOf<UBaseItem> ItemLogicClass;

    // Knife용 파라미터
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Knife", meta=(EditCondition="UseMode==EItemUseMode::Targeted"))
    float AttackRange = 200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Knife", meta=(EditCondition="UseMode==EItemUseMode::Targeted"))
    bool bRequiresLineOfSight = false;

    // Shield용 파라미터
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Shield", meta=(EditCondition="UseMode==EItemUseMode::SelfCast"))
    int32 BlockCount = 1;

};
