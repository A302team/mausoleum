#include "GamePlay/Items/ItemCursedSword.h"

#include "GameFramework/Character.h"
#include "Interface/A302AnimationBridge.h"
#include "Character/MyCharacter.h"
#include "Character/Components/Combat/EquipmentComponent.h"

bool UItemCursedSword::Use_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData)
{
	AMyCharacter* CharacterBridge = Cast<AMyCharacter>(Instigator);

	if (CharacterBridge)
	{
		CharacterBridge->SetTimedKnifeAttackInProgress(true);
	}

	const bool bUsed = Super::Use_Implementation(Instigator, TargetData);

	if (bUsed && CharacterBridge)
	{
		CharacterBridge->NotifyTimedKnifeAttackSucceeded();
	}

	if (CharacterBridge)
	{
		CharacterBridge->SetTimedKnifeAttackInProgress(false);
	}

	return bUsed;
}

void UItemCursedSword::PlayUsePresentation(ACharacter* Instigator)
{
	AMyCharacter* CharacterBridge = Cast<AMyCharacter>(Instigator);
	if (CharacterBridge && CharacterBridge->GetEquipmentComponent())
	{
		CharacterBridge->GetEquipmentComponent()->EquipCursedSwordWeapon();
	}

	if (!Instigator)
	{
		return;
	}

	if (IA302AnimationBridge* AnimBridge = Cast<IA302AnimationBridge>(Instigator->GetMesh() ? Instigator->GetMesh()->GetAnimInstance() : nullptr))
	{
		AnimBridge->PlayTimeKnifeAnimation();
	}
}
