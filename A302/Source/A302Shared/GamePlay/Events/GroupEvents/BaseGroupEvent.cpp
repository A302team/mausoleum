#include "GamePlay/Events/GroupEvents/BaseGroupEvent.h"
#include "A302GameplayGuards.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameMode/A302PlayerState.h"

void UBaseGroupEvent::InitializeContext(const URewardDefinition* InRewardDefinition, AActor* InSourceActor)
{
	RewardDefinition = InRewardDefinition;
	SourceActor = InSourceActor;
}

bool UBaseGroupEvent::IsParticipantEligible(const APlayerController* PlayerController) const
{
	if (!PlayerController || !PlayerController->GetPawn())
	{
		return false;
	}

	if (!A302GameplayGuards::DoesControllerMatchEventRoom(PlayerController, GetEventRoomCode()))
	{
		return false;
	}

	if (const AA302PlayerState* ExtendedPlayerState = PlayerController->GetPlayerState<AA302PlayerState>())
	{
		return ExtendedPlayerState->bIsAlive && !ExtendedPlayerState->bIsEscaped;
	}

	return true;
}
