#include "GamePlay/Events/PersonalEvents/PersonalEventInspectMalice.h"

#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"

void UPersonalEventInspectMalice::ExecuteEvent_Implementation(AMyCharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter)
	{
		return;
	}

	AMyPlayerController* PlayerController = Cast<AMyPlayerController>(InstigatorCharacter->GetController());
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PersonalEventInspectMalice] PlayerController is missing."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[PersonalEventInspectMalice] Opening inspect malice selection UI."));
	PlayerController->ShowInspectMaliceSelectionWidget();
	OnEventResolved(InstigatorCharacter, true);
}
