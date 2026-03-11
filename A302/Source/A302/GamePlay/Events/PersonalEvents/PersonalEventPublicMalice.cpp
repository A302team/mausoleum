#include "GamePlay/Events/PersonalEvents/PersonalEventPublicMalice.h"

#include "Character/Components/MaliceComponent.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameMode/A302GameInstance.h"

namespace
{
	FString ResolvePublicMaliceOwnerName(AMyCharacter* InstigatorCharacter)
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

		if (APlayerController* PlayerController = Cast<APlayerController>(InstigatorCharacter->GetController()))
		{
			if (PlayerController->IsLocalController())
			{
				if (UA302GameInstance* GameInstance = Cast<UA302GameInstance>(PlayerController->GetGameInstance()))
				{
					if (!GameInstance->MyPlayerName.IsEmpty())
					{
						return GameInstance->MyPlayerName;
					}
				}
			}
		}

		return GetNameSafe(InstigatorCharacter);
	}
}

void UPersonalEventPublicMalice::ExecuteEvent_Implementation(AMyCharacter* InstigatorCharacter)
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

	if (InstigatorCharacter->HasAuthority())
	{
		InstigatorCharacter->Multicast_ShowPublicMaliceAnnouncement(PlayerName, CurrentMaliceCount);
	}
	else if (AMyPlayerController* PlayerController = Cast<AMyPlayerController>(InstigatorCharacter->GetController()))
	{
		PlayerController->ShowPublicMaliceAnnouncement(PlayerName, CurrentMaliceCount);
	}

	OnEventResolved(InstigatorCharacter, true);
}
