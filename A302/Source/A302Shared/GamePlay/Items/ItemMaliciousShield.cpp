#include "GamePlay/Items/ItemMaliciousShield.h"
#include "Character/Components/MaliceComponent.h"
#include "GameFramework/Character.h"

void UItemMaliciousShield::OnItemAcquired(ACharacter* OwnerCharacter) const
{
	Super::OnItemAcquired(OwnerCharacter);

	if (OwnerCharacter)
	{
		if (UMaliceComponent* MaliceComp = OwnerCharacter->FindComponentByClass<UMaliceComponent>())
		{
			MaliceComp->ItemizeMalice += 1;
			MaliceComp->AddMalice(1); 
			UE_LOG(LogTemp, Warning, TEXT("[ItemMaliciousShield] 획득: 악의 +1"));
		}
	}
}

void UItemMaliciousShield::OnItemUsed(ACharacter* OwnerCharacter) const
{
	Super::OnItemUsed(OwnerCharacter);

	if (OwnerCharacter)
	{
		if (UMaliceComponent* MaliceComp = OwnerCharacter->FindComponentByClass<UMaliceComponent>())
		{
			MaliceComp->ItemizeMalice -= 1;
			MaliceComp->ConsumeMalice(1); 
			UE_LOG(LogTemp, Warning, TEXT("[ItemMaliciousShield] 사용: 악의 -1"));
		}
	}
}
