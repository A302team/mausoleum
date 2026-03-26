#include "GamePlay/Events/PersonalEvents/PersonalEventInspectMalice.h"

#include "Character/MyPlayerController.h"
#include "Character/Components/MaliceComponent.h"
#include "Character/Components/PlayerEventComponent.h"
#include "GameData/Events/PersonalEvents/Malice/PersonalEventInspectMaliceDefinition.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameMode/A302GameMode.h"
#include "GameMode/A302PlayerState.h"
#include "Room/RoomMembershipRegistry.h"
#include "Room/RoomScopeRules.h"

namespace
{
	constexpr int32 MaxInspectMaliceTargetCount = 5;

	FString ResolveInspectCandidateName(const APlayerController* PlayerController)
	{
		if (!PlayerController)
		{
			return TEXT("Unknown");
		}

		if (const APlayerState* PlayerState = PlayerController->GetPlayerState<APlayerState>())
		{
			const FString PlayerName = PlayerState->GetPlayerName();
			if (!PlayerName.IsEmpty())
			{
				return PlayerName;
			}
		}

		return PlayerController->GetName();
	}

	void GatherInspectMaliceCandidates(ACharacter* InstigatorCharacter, TArray<FInspectMaliceCandidateData>& OutCandidates)
	{
		OutCandidates.Reset();
		if (!InstigatorCharacter)
		{
			return;
		}

		UWorld* World = InstigatorCharacter->GetWorld();
		AMyPlayerController* OwnerController = Cast<AMyPlayerController>(InstigatorCharacter->GetController());
		const AA302PlayerState* OwnerPlayerState = InstigatorCharacter->GetPlayerState<AA302PlayerState>();
		if (!World || !OwnerController || !OwnerPlayerState)
		{
			return;
		}

		const FString RoomCode = A302RoomScope::NormalizeRoomCode(OwnerPlayerState->GetRoomCode());
		if (RoomCode.IsEmpty())
		{
			return;
		}

		AA302GameMode* GameMode = Cast<AA302GameMode>(World->GetAuthGameMode());
		URoomMembershipRegistry* Registry = GameMode ? GameMode->GetRoomMembershipRegistry() : nullptr;
		if (!Registry)
		{
			return;
		}

		TArray<APlayerController*> RoomPlayers;
		Registry->GatherPlayersInRoom(World, RoomCode, RoomPlayers);
		for (APlayerController* CandidateController : RoomPlayers)
		{
			if (!CandidateController || CandidateController == OwnerController)
			{
				continue;
			}

			const AA302PlayerState* CandidateState = CandidateController->GetPlayerState<AA302PlayerState>();
			if (CandidateState && (!CandidateState->bIsAlive || CandidateState->bIsEscaped))
			{
				continue;
			}

			const ACharacter* CandidateCharacter = Cast<ACharacter>(CandidateController->GetPawn());
			const UMaliceComponent* CandidateMaliceComponent = CandidateCharacter
				? CandidateCharacter->FindComponentByClass<UMaliceComponent>()
				: nullptr;

			FInspectMaliceCandidateData CandidateData;
			CandidateData.PlayerId = CandidateController->PlayerState ? CandidateController->PlayerState->GetPlayerId() : INDEX_NONE;
			CandidateData.PlayerName = ResolveInspectCandidateName(CandidateController);
			CandidateData.MaliceCount = CandidateMaliceComponent ? FMath::Max(0, CandidateMaliceComponent->GetMaliceCount()) : 0;
			OutCandidates.Add(CandidateData);

			if (OutCandidates.Num() >= MaxInspectMaliceTargetCount)
			{
				break;
			}
		}
	}
}

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

	TArray<FInspectMaliceCandidateData> CandidateData;
	GatherInspectMaliceCandidates(InstigatorCharacter, CandidateData);
	if (CandidateData.Num() > 0)
	{
		EventComp->ShowInspectMaliceSelectionWidgetWithCandidatesAndConfig(CandidateData, SelectionTimeoutSeconds, ResultDisplaySeconds);
	}
	else
	{
		EventComp->ShowInspectMaliceSelectionWidgetWithConfig(SelectionTimeoutSeconds, ResultDisplaySeconds);
	}

	OnEventResolved_Implementation(InstigatorCharacter, true);
}
