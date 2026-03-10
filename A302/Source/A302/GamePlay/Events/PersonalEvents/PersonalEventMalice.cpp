#include "GamePlay/Events/PersonalEvents/PersonalEventMalice.h"

#include "Character/Components/MaliceComponent.h"
#include "Character/MyCharacter.h"
#include "GameData/ItemDefinition.h"

void UPersonalEventMalice::ExecuteEvent_Implementation(AMyCharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter)
	{
		return;
	}

	const UItemDefinition* ItemDef = GetRewardDefinition();
	const int32 MaliceAmount = ItemDef ? FMath::Max(1, ItemDef->MaliceAmount) : 1;

	if (UMaliceComponent* MaliceComponent = InstigatorCharacter->FindComponentByClass<UMaliceComponent>())
	{
		MaliceComponent->AddMalice(MaliceAmount);
		UE_LOG(LogTemp, Log, TEXT("[PersonalEventMalice] Added malice: +%d"), MaliceAmount);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PersonalEventMalice] MaliceComponent missing."));
	}

	OnEventResolved(InstigatorCharacter, true);
}
