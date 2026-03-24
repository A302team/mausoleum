#include "GamePlay/Events/PersonalEvents/PersonalEventInspectMalice.h"

#include "Character/MyPlayerController.h"
#include "Character/Components/PlayerEventComponent.h"
#include "GameData/Events/PersonalEvents/Malice/PersonalEventInspectMaliceDefinition.h"

void UPersonalEventInspectMalice::ExecuteEvent_Implementation(ACharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter || !InstigatorCharacter->HasAuthority())
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

	float SelectionTimeoutSeconds = 10.0f;
	float ResultDisplaySeconds = 3.0f;

	if (const UPersonalEventInspectMaliceDefinition* InspectDef = Cast<UPersonalEventInspectMaliceDefinition>(GetRewardDefinition()))
	{
		SelectionTimeoutSeconds = FMath::Max(0.1f, InspectDef->Payload.SelectionTimeoutSeconds);
		ResultDisplaySeconds = FMath::Max(0.1f, InspectDef->Payload.ResultDisplaySeconds);
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[PersonalEventInspectMalice] Requesting inspect malice selection UI on owning client. selectionTimeout=%.1fs resultDisplay=%.1fs"),
		SelectionTimeoutSeconds,
		ResultDisplaySeconds
	);
	EventComp->ShowInspectMaliceSelectionWidgetWithConfig(SelectionTimeoutSeconds, ResultDisplaySeconds);
	OnEventResolved_Implementation(InstigatorCharacter, true);
}
