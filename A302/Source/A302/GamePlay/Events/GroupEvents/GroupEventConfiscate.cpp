#include "GamePlay/Events/GroupEvents/GroupEventConfiscate.h"

#include "Character/Components/ItemManagerComponent.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "Engine/World.h"
#include "GameData/Events/GroupEvents/GroupEventConfiscateDefinition.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameMode/A302PlayerState.h"

namespace
{
	FString ResolveVotePlayerName(const APlayerController* PlayerController)
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

	bool IsGroupEventParticipant(const APlayerController* PlayerController)
	{
		if (!PlayerController || !PlayerController->GetPawn())
		{
			return false;
		}

		const APlayerState* BasePlayerState = PlayerController->GetPlayerState<APlayerState>();
		if (!BasePlayerState)
		{
			return false;
		}

		if (const AA302PlayerState* ExtendedPlayerState = Cast<AA302PlayerState>(BasePlayerState))
		{
			return ExtendedPlayerState->bIsAlive && !ExtendedPlayerState->bIsEscaped;
		}

		return true;
	}
}

void UGroupEventConfiscate::ExecuteEvent_Implementation(AMyCharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter || !InstigatorCharacter->HasAuthority() || !GetWorld() || bResolved)
	{
		return;
	}

	Participants.Reset();
	SubmittedVotes.Reset();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AMyPlayerController* PlayerController = Cast<AMyPlayerController>(It->Get());
		AMyCharacter* Character = PlayerController ? Cast<AMyCharacter>(PlayerController->GetPawn()) : nullptr;
		if (!PlayerController || !Character || !IsGroupEventParticipant(PlayerController))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[GroupEventVote] Skipped participant. controller=%s pawn=%s playerState=%s"),
				*GetNameSafe(PlayerController),
				*GetNameSafe(Character),
				*GetNameSafe(PlayerController ? PlayerController->GetPlayerState<APlayerState>() : nullptr)
			);
			continue;
		}

		PlayerController->ActiveGroupEvent = this;
		Participants.Add(PlayerController);
	}

	if (Participants.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GroupEventVote] No participants found for event %s"), *EventID.ToString());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[GroupEventVote] Starting vote. participants=%d event=%s"), Participants.Num(), *EventID.ToString());

	const URewardDefinition* EventRewardDefinition = GetRewardDefinition();
	const FText Title = EventRewardDefinition && !EventRewardDefinition->DisplayName.IsEmpty()
		? EventRewardDefinition->DisplayName
		: FText::FromString(TEXT("Vote"));
	const FText Description = EventRewardDefinition && !EventRewardDefinition->Description.IsEmpty()
		? EventRewardDefinition->Description
		: FText::FromString(TEXT("가장 많은 선택을 받은 유저의 모든 아이템을 몰수합니다."));
	const float VoteDuration = static_cast<float>(ResolveVoteDurationSeconds());

	for (AMyPlayerController* PlayerController : Participants)
	{
		if (PlayerController)
		{
			PlayerController->Client_OpenGroupEventVote(EventID, Title, Description, VoteDuration);
		}
	}

	GetWorld()->GetTimerManager().ClearTimer(VoteTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		VoteTimerHandle,
		this,
		&UGroupEventConfiscate::ResolveVote,
		VoteDuration,
		false
	);
}

bool UGroupEventConfiscate::SubmitVote(AMyPlayerController* VotingPlayerController, int32 TargetPlayerId)
{
	if (bResolved || !VotingPlayerController || !Participants.Contains(VotingPlayerController))
	{
		return false;
	}

	if (SubmittedVotes.Contains(VotingPlayerController))
	{
		return false;
	}

	bool bIsValidTarget = false;
	for (AMyPlayerController* Participant : Participants)
	{
		const APlayerState* ParticipantState = Participant ? Participant->GetPlayerState<APlayerState>() : nullptr;
		if (ParticipantState && ParticipantState->GetPlayerId() == TargetPlayerId)
		{
			bIsValidTarget = true;
			break;
		}
	}

	if (!bIsValidTarget)
	{
		return false;
	}

	SubmittedVotes.Add(VotingPlayerController, TargetPlayerId);

	if (SubmittedVotes.Num() >= Participants.Num())
	{
		ResolveVote();
	}

	return true;
}

void UGroupEventConfiscate::ResolveVote()
{
	if (bResolved)
	{
		return;
	}

	bResolved = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(VoteTimerHandle);
	}

	if (SubmittedVotes.Num() == 0)
	{
		FinishEvent(FText::FromString(TEXT("아무도 투표하지 않아 몰수가 진행되지 않았습니다.")));
		return;
	}

	TMap<int32, int32> VoteCounts;
	for (const TPair<TObjectPtr<AMyPlayerController>, int32>& VoteEntry : SubmittedVotes)
	{
		VoteCounts.FindOrAdd(VoteEntry.Value)++;
	}

	int32 MaxVotes = 0;
	TArray<int32> TopPlayerIds;
	for (const TPair<int32, int32>& VoteCountEntry : VoteCounts)
	{
		if (VoteCountEntry.Value > MaxVotes)
		{
			MaxVotes = VoteCountEntry.Value;
			TopPlayerIds.Reset();
			TopPlayerIds.Add(VoteCountEntry.Key);
		}
		else if (VoteCountEntry.Value == MaxVotes)
		{
			TopPlayerIds.Add(VoteCountEntry.Key);
		}
	}

	if (TopPlayerIds.Num() == 0)
	{
		FinishEvent(FText::FromString(TEXT("몰수 대상을 결정하지 못했습니다.")));
		return;
	}

	const int32 SelectedPlayerId = TopPlayerIds[FMath::RandRange(0, TopPlayerIds.Num() - 1)];

	AMyPlayerController* TargetPlayerController = nullptr;
	for (AMyPlayerController* Participant : Participants)
	{
		const APlayerState* PlayerState = Participant ? Participant->GetPlayerState<APlayerState>() : nullptr;
		if (PlayerState && PlayerState->GetPlayerId() == SelectedPlayerId)
		{
			TargetPlayerController = Participant;
			break;
		}
	}

	if (!TargetPlayerController)
	{
		FinishEvent(FText::FromString(TEXT("몰수 대상을 찾지 못했습니다.")));
		return;
	}

	int32 RemovedItemCount = 0;
	if (AMyCharacter* TargetCharacter = Cast<AMyCharacter>(TargetPlayerController->GetPawn()))
	{
		if (UItemManagerComponent* ItemManager = TargetCharacter->FindComponentByClass<UItemManagerComponent>())
		{
			RemovedItemCount = ItemManager->RemoveAllItems();
		}
	}

	TargetPlayerController->Client_ApplyConfiscationToLocalInventory();

	const FText ResultText = FText::FromString(
		FString::Printf(
			TEXT("%s의 퀵슬롯 아이템 %d개를 몰수했습니다."),
			*ResolveVotePlayerName(TargetPlayerController),
			RemovedItemCount
		)
	);

	FinishEvent(ResultText);
}

void UGroupEventConfiscate::FinishEvent(const FText& ResultText)
{
	for (AMyPlayerController* Participant : Participants)
	{
		if (!Participant)
		{
			continue;
		}

		Participant->Client_FinishGroupEventVote(EventID, ResultText);
		Participant->ActiveGroupEvent = nullptr;
	}

	Participants.Reset();
	SubmittedVotes.Reset();
}

int32 UGroupEventConfiscate::ResolveVoteDurationSeconds() const
{
	const UGroupEventConfiscateDefinition* EventDefinition =
		Cast<UGroupEventConfiscateDefinition>(const_cast<URewardDefinition*>(GetRewardDefinition()));
	if (!EventDefinition)
	{
		return 10;
	}

	return FMath::Max(1, FMath::RoundToInt(EventDefinition->Payload.VoteDurationSeconds));
}
