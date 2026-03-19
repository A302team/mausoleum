#include "GamePlay/Events/PersonalEvents/PersonalEventInspectMalice.h"

#include "Character/MyPlayerController.h"

void UPersonalEventInspectMalice::ExecuteEvent_Implementation(ACharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter)
	{
		return;
	}

	AMyPlayerController* ClientEventBridge = Cast<AMyPlayerController>(InstigatorCharacter->GetController());
	if (!ClientEventBridge)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PersonalEventInspectMalice] ClientEventBridge is missing."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[PersonalEventInspectMalice] Requesting inspect malice selection UI on owning client."));
	ClientEventBridge->ShowInspectMaliceSelectionWidget();
	OnEventResolved(InstigatorCharacter, true);
}
