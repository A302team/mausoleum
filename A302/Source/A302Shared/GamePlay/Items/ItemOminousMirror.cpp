#include "GamePlay/Items/ItemOminousMirror.h"

#include "Character/Components/MaliceComponent.h"
#include "Character/MyCharacter.h"
#include "GameData/Items/ItemDefinition.h"
#include "GameData/Items/ItemInstance.h"
#include "A302GameplayGuards.h"

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

	// 서버에서 직접 효과가 적용되어야 하므로 이 부분은 삭제하거나 서버 전용 로직 호출
	// OwnerCharacter가 AMyCharacter인 경우 필요한 로직이 있다면 여기서 직접 수행하거나
	// ResolveServerTargetedUse가 나중에 호출되도록 보장해야 합니다.

	if (UItemInstance* Inst = GetInstance())
	{
		Inst->Consume(1);
	}

	OnItemUsed(OwnerCharacter);
	return true;
}

bool UItemOminousMirror::ResolveServerTargetedUse(
	ACharacter* InOwnerCharacter,
	AActor* TargetActor,
	FString& OutSystemMessage
) const
{
	OutSystemMessage.Reset();

	AMyCharacter* OwnerCharacter = Cast<AMyCharacter>(InOwnerCharacter);
	AMyCharacter* TargetCharacter = Cast<AMyCharacter>(TargetActor);
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority() || !TargetCharacter || TargetCharacter == OwnerCharacter)
	{
		return false;
	}

	if (!A302GameplayGuards::CanInstigatorAffectTargetActor(OwnerCharacter, TargetCharacter))
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
