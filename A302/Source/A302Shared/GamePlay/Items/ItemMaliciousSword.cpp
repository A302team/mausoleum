#include "GamePlay/Items/ItemMaliciousSword.h"
#include "Character/Components/MaliceComponent.h"
#include "GameFramework/Character.h"

void UItemMaliciousSword::OnItemAcquired(ACharacter* OwnerCharacter) const
{
	Super::OnItemAcquired(OwnerCharacter);

	if (OwnerCharacter)
	{
		if (UMaliceComponent* MaliceComp = OwnerCharacter->FindComponentByClass<UMaliceComponent>())
		{
			MaliceComp->ItemizeMalice += 1;
			MaliceComp->AddMalice(1); 
			UE_LOG(LogTemp, Warning, TEXT("[ItemMaliciousSword] 획득: 악의 +1"));
		}
	}
}

void UItemMaliciousSword::OnItemUsed(ACharacter* OwnerCharacter) const
{
	Super::OnItemUsed(OwnerCharacter);

	if (OwnerCharacter)
	{
		if (UMaliceComponent* MaliceComp = OwnerCharacter->FindComponentByClass<UMaliceComponent>())
		{
			MaliceComp->ItemizeMalice -= 1;
			MaliceComp->ConsumeMalice(1); 
			UE_LOG(LogTemp, Warning, TEXT("[ItemMaliciousSword] 사용: 악의 -1"));
		}
	}
}
