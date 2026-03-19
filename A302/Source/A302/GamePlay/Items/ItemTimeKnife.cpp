#include "GamePlay/Items/ItemTimeKnife.h"

#include "Animation/MyAnimInstance.h"
#include "Character/MyCharacter.h"

bool UItemTimeKnife::Use_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData)
{
	AMyCharacter* MyCharacter = Cast<AMyCharacter>(Instigator);
	if (MyCharacter)
	{
		MyCharacter->SetTimedKnifeAttackInProgress(true);
	}

	const bool bUsed = Super::Use_Implementation(Instigator, TargetData);

	if (bUsed && MyCharacter)
	{
		MyCharacter->NotifyTimedKnifeAttackSucceeded();
	}

	if (MyCharacter)
	{
		MyCharacter->SetTimedKnifeAttackInProgress(false);
	}

	return bUsed;
}

void UItemTimeKnife::PlayUsePresentation(ACharacter* Instigator)
{
	AMyCharacter* MyCharacter = Cast<AMyCharacter>(Instigator);
	if (!MyCharacter)
	{
		return;
	}

	if (MyCharacter->TimeKnifeActorClass)
	{
		MyCharacter->EquipWeapon(MyCharacter->TimeKnifeActorClass);
	}

	if (UMyAnimInstance* Anim = Cast<UMyAnimInstance>(MyCharacter->GetMesh()->GetAnimInstance()))
	{
		Anim->PlayTimeKnifeMontage();
	}
}
