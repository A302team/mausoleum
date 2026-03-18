#include "GamePlay/Items/ItemOminousMirror.h"

#include "Character/Components/MaliceComponent.h"
#include "Character/MyCharacter.h"
#include "GameData/Items/ItemDefinition.h"
#include "GameData/Items/ItemInstance.h"

bool UItemOminousMirror::CanUse_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData) const
{
	const AMyCharacter* OwnerCharacter = Cast<AMyCharacter>(Instigator);
	const AMyCharacter* TargetCharacter = Cast<AMyCharacter>(TargetData.TargetActor);
	return OwnerCharacter && TargetCharacter && OwnerCharacter != TargetCharacter;
}

bool UItemOminousMirror::Use_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData)
{
	if (!CanUse_Implementation(Instigator, TargetData))
	{
		return false;
	}

	AMyCharacter* OwnerCharacter = Cast<AMyCharacter>(Instigator);
	AMyCharacter* TargetCharacter = Cast<AMyCharacter>(TargetData.TargetActor);
	if (!OwnerCharacter || !TargetCharacter)
	{
		return false;
	}

	const UItemDefinition* ItemDefinition = GetDefinition();
	if (!ItemDefinition)
	{
		return false;
	}

	OwnerCharacter->Server_RequestTargetedItemUse(const_cast<UItemDefinition*>(ItemDefinition), TargetCharacter);

	if (UItemInstance* Inst = GetInstance())
	{
		Inst->Consume(1);
	}

	OnItemUsed(OwnerCharacter);
	return true;
}

bool UItemOminousMirror::ResolveServerTargetedUse(
	AMyCharacter* OwnerCharacter,
	AActor* TargetActor,
	FString& OutSystemMessage
) const
{
	OutSystemMessage.Reset();

	AMyCharacter* TargetCharacter = Cast<AMyCharacter>(TargetActor);
	if (!OwnerCharacter || !TargetCharacter || TargetCharacter == OwnerCharacter)
	{
		return false;
	}

	UMaliceComponent* OwnerMaliceComponent = OwnerCharacter->FindComponentByClass<UMaliceComponent>();
	UMaliceComponent* TargetMaliceComponent = TargetCharacter->FindComponentByClass<UMaliceComponent>();
	if (!OwnerMaliceComponent || !TargetMaliceComponent)
	{
		return false;
	}

	const int32 OwnerRawMalice = FMath::Max(0, OwnerMaliceComponent->GetRawMalice());
	const int32 TargetRawMalice = FMath::Max(0, TargetMaliceComponent->GetRawMalice());

	OwnerMaliceComponent->SetRawMalice(TargetRawMalice);
	TargetMaliceComponent->SetRawMalice(OwnerRawMalice);
	return true;
}
