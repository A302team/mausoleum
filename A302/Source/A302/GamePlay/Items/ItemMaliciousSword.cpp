#include "GamePlay/Items/ItemMaliciousSword.h"
#include "Character/MyCharacter.h"
#include "Character/Components/MaliceComponent.h"

void UItemMaliciousSword::OnItemAcquired(AMyCharacter* OwnerCharacter) const
{
	Super::OnItemAcquired(OwnerCharacter);

	if (OwnerCharacter)
	{
		if (UMaliceComponent* MaliceComp = OwnerCharacter->FindComponentByClass<UMaliceComponent>())
		{
			MaliceComp->AddMalice(1); 
			UE_LOG(LogTemp, Warning, TEXT("[ItemMaliciousSword] 획득: 악의 +1"));
		}
	}
}

void UItemMaliciousSword::OnItemUsed(AMyCharacter* OwnerCharacter) const
{
	Super::OnItemUsed(OwnerCharacter);

	if (OwnerCharacter)
	{
		if (UMaliceComponent* MaliceComp = OwnerCharacter->FindComponentByClass<UMaliceComponent>())
		{
			MaliceComp->ConsumeMalice(1); 
			UE_LOG(LogTemp, Warning, TEXT("[ItemMaliciousSword] 사용: 악의 -1"));
		}
	}
}