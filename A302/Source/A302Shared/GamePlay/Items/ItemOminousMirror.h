#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Items/BaseItem.h"
#include "Interface/UsableItem.h"
#include "ItemOminousMirror.generated.h"

UCLASS(Blueprintable)
class A302SHARED_API UItemOminousMirror : public UBaseItem, public IUsableItem
{
	GENERATED_BODY()

public:
	virtual bool CanUse_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData) const override;
	virtual bool Use_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData) override;
	virtual bool ResolveServerTargetedUse(
		AMyCharacter* OwnerCharacter,
		AActor* TargetActor,
		FString& OutSystemMessage
	) const override;
};
