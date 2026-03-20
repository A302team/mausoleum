#include "GamePlay/Items/ItemSereneLantern.h"

#include "Character/Components/MaliceComponent.h"
#include "Character/MyCharacter.h"
#include "GameData/Items/ItemDefinition.h"
#include "GameData/Items/ItemInstance.h"
#include "A302GameplayGuards.h"

bool UItemSereneLantern::CanUse_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData) const
{
	const AMyCharacter* OwnerCharacter = Cast<AMyCharacter>(Instigator);
	const AMyCharacter* TargetCharacter = TargetData.TargetActor
		? Cast<AMyCharacter>(TargetData.TargetActor)
		: OwnerCharacter;

	return OwnerCharacter && TargetCharacter;
}

bool UItemSereneLantern::Use_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData)
{
	if (!CanUse_Implementation(Instigator, TargetData))
	{
		return false;
	}

	AMyCharacter* OwnerCharacter = Cast<AMyCharacter>(Instigator);
	AMyCharacter* TargetCharacter = TargetData.TargetActor
		? Cast<AMyCharacter>(TargetData.TargetActor)
		: OwnerCharacter;
	if (!OwnerCharacter || !TargetCharacter)
	{
		return false;
	}

	const UItemDefinition* ItemDefinition = GetDefinition();
	if (!ItemDefinition)
	{
		return false;
	}

	if (UItemInstance* Inst = GetInstance())
	{
		Inst->Consume(1);
	}

	OnItemUsed(OwnerCharacter);
	return true;
}

bool UItemSereneLantern::ResolveServerTargetedUse(
	ACharacter* OwnerCharacter,
	AActor* TargetActor,
	FString& OutSystemMessage
) const
{
	OutSystemMessage.Reset();

	AMyCharacter* TargetCharacter = Cast<AMyCharacter>(TargetActor);
	if (!TargetCharacter)
	{
		TargetCharacter = Cast<AMyCharacter>(OwnerCharacter);
	}

	if (!OwnerCharacter || !TargetCharacter)
	{
		return false;
	}

	if (!OwnerCharacter->HasAuthority())
	{
		return false;
	}

	if (TargetCharacter != OwnerCharacter && !A302GameplayGuards::CanInstigatorAffectTargetActor(OwnerCharacter, TargetCharacter))
	{
		return false;
	}

	UMaliceComponent* TargetMaliceComponent = TargetCharacter->FindComponentByClass<UMaliceComponent>();
	if (!TargetMaliceComponent)
	{
		return false;
	}

	const int32 TargetRawMalice = FMath::Max(0, TargetMaliceComponent->GetRawMalice());
	TargetMaliceComponent->SetRawMalice(FMath::Max(0, TargetRawMalice - 2));
	return true;
}
