#pragma once

#include "CoreMinimal.h"
#include "Character/Items/BaseItem.h"
#include "Interface/UsableItem.h"
#include "ItemKnife.generated.h"

UCLASS(BlueprintType)
class MYGAME_API UItemKnife : public UBaseItem, public IUsableItem
{
    GENERATED_BODY()

public:
    virtual bool CanUse_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData) const override;
    virtual bool Use_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData) override;

private:
    bool HasLineOfSight(ACharacter* Instigator, AActor* Target) const;
};