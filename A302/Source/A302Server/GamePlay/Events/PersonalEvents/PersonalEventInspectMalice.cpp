#include "GamePlay/Events/PersonalEvents/PersonalEventInspectMalice.h"

#include "Character/MyPlayerController.h"
#include "Character/Components/PlayerEventComponent.h"

void UPersonalEventInspectMalice::ExecuteEvent_Implementation(ACharacter* InstigatorCharacter)
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

	UPlayerEventComponent* EventComp = PlayerController->GetPlayerEventComponent();
	if (!EventComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PersonalEventInspectMalice] PlayerEventComponent is missing."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[PersonalEventInspectMalice] Requesting inspect malice selection UI on owning client."));
	EventComp->ShowInspectMaliceSelectionWidget();
	OnEventResolved(InstigatorCharacter, true);
}
