#pragma once

#include "CoreMinimal.h"
#include "Character/Items/BaseItem.h"
#include "Interface/UsableItem.h"
#include "ItemShield.generated.h"

class UCombatStatusComponent;

UCLASS(BlueprintType)
class MYGAME_API UItemShield : public UBaseItem, public IUsableItem
{
    GENERATED_BODY()

public:
    virtual bool CanUse_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData) const override;
    virtual bool Use_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData) override;
};