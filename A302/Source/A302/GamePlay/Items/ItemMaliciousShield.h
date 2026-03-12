#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Items/ItemShield.h"
#include "ItemMaliciousShield.generated.h"

UCLASS(Blueprintable)
class A302_API UItemMaliciousShield : public UItemShield
{
	GENERATED_BODY()

public:
	// 획득 시 악의 +1
	virtual void OnItemAcquired(class AMyCharacter* OwnerCharacter) const override;
    
	// 사용 시 악의 -1
	virtual void OnItemUsed(class AMyCharacter* OwnerCharacter) const override;
};