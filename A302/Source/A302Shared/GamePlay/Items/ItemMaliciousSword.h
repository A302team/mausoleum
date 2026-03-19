#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Items/ItemKnife.h"
#include "ItemMaliciousSword.generated.h"

UCLASS(Blueprintable)
class A302SHARED_API UItemMaliciousSword : public UItemKnife
{
	GENERATED_BODY()

public:
	// 획득 시 악의 +1
	virtual void OnItemAcquired(class ACharacter* OwnerCharacter) const override;
    
	// 사용 시 악의 -1
	virtual void OnItemUsed(class ACharacter* OwnerCharacter) const override;
};
