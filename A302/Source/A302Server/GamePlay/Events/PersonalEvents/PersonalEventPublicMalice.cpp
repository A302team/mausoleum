#include "GamePlay/Events/PersonalEvents/PersonalEventPublicMalice.h"

#include "Character/Components/MaliceComponent.h"
#include "GameFramework/PlayerState.h"
#include "Interface/A302CharacterBridge.h"
#include "Interface/A302ClientEventBridge.h"

namespace
{
	FString ResolvePublicMaliceOwnerName(ACharacter* InstigatorCharacter)
	{
		if (!InstigatorCharacter)
		{
			return TEXT("Unknown");
		}

		if (APlayerState* PlayerState = InstigatorCharacter->GetPlayerState())
		{
			const FString PlayerName = PlayerState->GetPlayerName();
			if (!PlayerName.IsEmpty())
			{
				return PlayerName;
			}
		}

		return GetNameSafe(InstigatorCharacter);
	}
}

void UPersonalEventPublicMalice::ExecuteEvent_Implementation(ACharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter)
	{
		return;
	}

	const UMaliceComponent* MaliceComponent = InstigatorCharacter->FindComponentByClass<UMaliceComponent>();
	const int32 CurrentMaliceCount = MaliceComponent ? FMath::Max(0, MaliceComponent->MaliceCount) : 0;
	const FString PlayerName = ResolvePublicMaliceOwnerName(InstigatorCharacter);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[PersonalEventPublicMalice] Broadcasting public malice. player=%s malice=%d"),
		*PlayerName,
		CurrentMaliceCount
	);

	if (IA302CharacterBridge* CharacterBridge = Cast<IA302CharacterBridge>(InstigatorCharacter))
	{
		CharacterBridge->BroadcastPublicMaliceAnnouncement(PlayerName, CurrentMaliceCount);
	}
	else if (IA302ClientEventBridge* ClientEventBridge = Cast<IA302ClientEventBridge>(InstigatorCharacter->GetController()))
	{
		ClientEventBridge->ShowPublicMaliceAnnouncement(PlayerName, CurrentMaliceCount);
	}

	OnEventResolved(InstigatorCharacter, true);
}
