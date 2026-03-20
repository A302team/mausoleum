#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Items/BaseItem.h"
#include "Interface/UsableItem.h"
#include "ItemKnife.generated.h"

UCLASS(BlueprintType)
class A302SHARED_API UItemKnife : public UBaseItem, public IUsableItem
{
    GENERATED_BODY()

public:
    virtual bool CanUse_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData) const override;
    virtual bool Use_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData) override;
    virtual bool ResolveServerTargetedUse(ACharacter* OwnerCharacter, AActor* TargetActor, FString& OutSystemMessage) const override;

protected:
    virtual void PlayUsePresentation(ACharacter* Instigator);

private:
    bool HasLineOfSight(ACharacter* Instigator, AActor* Target) const;
};
