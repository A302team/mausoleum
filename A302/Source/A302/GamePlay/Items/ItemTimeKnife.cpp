#include "GamePlay/Items/ItemTimeKnife.h"

#include "Character/MyCharacter.h"

bool UItemTimeKnife::Use_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData)
{
	AMyCharacter* MyCharacter = Cast<AMyCharacter>(Instigator);
	if (MyCharacter)
	{
		MyCharacter->SetTimedKnifeAttackInProgress(true);

		// TimeKnifeActor 장착 (애니메이션에서 위치 참조용)
        if (MyCharacter->TimeKnifeActorClass)
        {
            MyCharacter->EquipWeapon(MyCharacter->TimeKnifeActorClass);
        }
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
