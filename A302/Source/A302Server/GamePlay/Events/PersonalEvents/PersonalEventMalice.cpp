#include "GamePlay/Events/PersonalEvents/PersonalEventMalice.h"

#include "Character/Components/MaliceComponent.h"
#include "GameData/Events/PersonalEvents/Malice/PersonalEventMaliceDefinition.h"
#include "GameData/RewardDefinition.h"

void UPersonalEventMalice::ExecuteEvent_Implementation(ACharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter)
	{
		return;
	}

	const UPersonalEventMaliceDefinition* EventDef =
		Cast<UPersonalEventMaliceDefinition>(const_cast<URewardDefinition*>(GetRewardDefinition()));
	const int32 MaliceAmount = EventDef ? FMath::Max(1, EventDef->Payload.MaliceAmount) : 1;

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

