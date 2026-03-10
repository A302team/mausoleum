#include "GamePlay/Items/ItemTimeKnife.h"

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
