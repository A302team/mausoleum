#include "GamePlay/Events/PersonalEvents/PersonalEventPhase1Collect.h"

void UPersonalEventPhase1Collect::ExecuteEvent_Implementation(ACharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter || !InstigatorCharacter->HasAuthority())
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[PersonalEventPhase1Collect] Collected by %s. Reward resolved for phase progression."),
		*GetNameSafe(InstigatorCharacter)
	);

	// This event is intentionally instant: pickup == resolved.
	OnEventResolved_Implementation(InstigatorCharacter, true);
}
