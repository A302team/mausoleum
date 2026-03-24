#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Items/ItemKnife.h"
#include "ItemCursedSword.generated.h"

UCLASS(BlueprintType)
class A302SHARED_API UItemCursedSword : public UItemKnife
{
	GENERATED_BODY()

public:
	virtual void OnItemAcquired(class ACharacter* OwnerCharacter) const override;
	virtual bool Use_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData) override;

protected:
	virtual void PlayUsePresentation(ACharacter* Instigator) override;
};
