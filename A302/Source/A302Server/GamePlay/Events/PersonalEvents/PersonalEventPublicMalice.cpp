#include "GamePlay/Events/PersonalEvents/PersonalEventPublicMalice.h"

#include "Character/Components/MaliceComponent.h"
#include "GameFramework/PlayerState.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"

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
	if (!InstigatorCharacter || !InstigatorCharacter->HasAuthority())
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

	if (AMyCharacter* CharacterBridge = Cast<AMyCharacter>(InstigatorCharacter))
	{
		CharacterBridge->BroadcastPublicMaliceAnnouncement(PlayerName, CurrentMaliceCount);
	}
	else if (AMyPlayerController* ClientEventBridge = Cast<AMyPlayerController>(InstigatorCharacter->GetController()))
	{
		ClientEventBridge->ShowPublicMaliceAnnouncement(PlayerName, CurrentMaliceCount);
	}

	OnEventResolved_Implementation(InstigatorCharacter, true);
}
