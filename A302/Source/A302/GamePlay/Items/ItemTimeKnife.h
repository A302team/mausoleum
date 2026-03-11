#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Items/ItemKnife.h"
#include "ItemTimeKnife.generated.h"

UCLASS(BlueprintType)
class A302_API UItemTimeKnife : public UItemKnife
{
	GENERATED_BODY()

public:
	virtual bool Use_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData) override;
};

