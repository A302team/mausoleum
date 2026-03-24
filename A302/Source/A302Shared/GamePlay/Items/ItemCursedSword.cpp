#include "GamePlay/Items/ItemCursedSword.h"

#include "GameFramework/Character.h"
#include "Interface/A302AnimationBridge.h"
#include "Character/MyCharacter.h"
#include "Character/Components/PlayerEventComponent.h"
#include "GameData/Items/ItemDefinition.h"
#include "Character/Components/Combat/EquipmentComponent.h"

void UItemCursedSword::OnItemAcquired(ACharacter* OwnerCharacter) const
{
	Super::OnItemAcquired(OwnerCharacter);

	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}

	const UItemDefinition* Definition = GetDefinition();
	if (!Definition || Definition->ItemId.IsNone())
	{
		return;
	}

	if (UPlayerEventComponent* PlayerEventComponent = OwnerCharacter->FindComponentByClass<UPlayerEventComponent>())
	{
		constexpr float TimedKnifeDurationSeconds = 10.0f;
		const FText Title = Definition->DisplayName.IsEmpty()
			? FText::FromString(TEXT("저주받은 검"))
			: Definition->DisplayName;
		PlayerEventComponent->StartTimedKnifeCountdown(TimedKnifeDurationSeconds, Definition->ItemId, Title);
	}
}

bool UItemCursedSword::Use_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData)
{
	AMyCharacter* CharacterBridge = Cast<AMyCharacter>(Instigator);

	if (CharacterBridge)
	{
		CharacterBridge->SetTimedKnifeAttackInProgress(true);
	}

	const bool bUsed = Super::Use_Implementation(Instigator, TargetData);
	// Timed-kill completion must be confirmed by an actual kill event,
	// not by a successful attack input/use attempt.

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
