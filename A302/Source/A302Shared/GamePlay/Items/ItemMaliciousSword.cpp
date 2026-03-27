#include "GamePlay/Items/ItemMaliciousSword.h"
#include "Character/Components/MaliceComponent.h"
#include "GameFramework/Character.h"

void UItemMaliciousSword::OnItemAcquired(ACharacter* OwnerCharacter) const
{
	Super::OnItemAcquired(OwnerCharacter);

	if (OwnerCharacter && OwnerCharacter->HasAuthority())
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

	if (OwnerCharacter && OwnerCharacter->HasAuthority())
	{
		if (UMaliceComponent* MaliceComp = OwnerCharacter->FindComponentByClass<UMaliceComponent>())
		{
			MaliceComp->ItemizeMalice = FMath::Max(0, MaliceComp->ItemizeMalice - 1);
			MaliceComp->ConsumeMalice(1);
			UE_LOG(LogTemp, Warning, TEXT("[ItemMaliciousSword] 사용: 악의 -1, 아이템화 악의 -1"));
		}
	}
}
