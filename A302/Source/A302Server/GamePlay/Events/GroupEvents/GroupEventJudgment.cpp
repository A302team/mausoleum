#include "GamePlay/Events/GroupEvents/GroupEventJudgment.h"

#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "Character/Components/Combat/CharacterHealthComponent.h"
#include "Character/Components/MaliceComponent.h"
#include "Engine/World.h"
#include "GameData/Events/GroupEvents/GroupEventJudgmentDefinition.h"
#include "GameData/RewardDefinition.h"
#include "GameMode/A302GameMode.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Room/RoomMembershipRegistry.h"

namespace
{
	FString ResolveJudgmentVotePlayerName(const APlayerController* PlayerController)
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

	void GatherRoomScopedVoteCandidates(const UGroupEventJudgment* GroupEvent, ACharacter* InstigatorCharacter, TArray<APlayerController*>& OutCandidates)
	{
		OutCandidates.Reset();

		if (!GroupEvent)
		{
			return;
		}

		UWorld* World = GroupEvent->GetWorld();
		if (!World)
		{
			return;
		}

		if (AA302GameMode* GameMode = Cast<AA302GameMode>(World->GetAuthGameMode()))
		{
			if (URoomMembershipRegistry* Registry = GameMode->GetRoomMembershipRegistry())
			{
				Registry->GatherPlayersInRoom(World, GroupEvent->GetEventRoomCode(), OutCandidates);
				return;
			}
		}

		if (OutCandidates.Num() == 0 && InstigatorCharacter)
		{
			if (APlayerController* InstigatorController = Cast<APlayerController>(InstigatorCharacter->GetController()))
			{
				OutCandidates.AddUnique(InstigatorController);
			}
		}
	}
}

void UGroupEventJudgment::ExecuteEvent_Implementation(ACharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter || !InstigatorCharacter->HasAuthority() || !GetWorld() || bResolved)
	{
		return;
	}

	Participants.Reset();
	SubmittedVotes.Reset();
	InitializeRuntimeContext(InstigatorCharacter);

	TArray<APlayerController*> CandidateControllers;
	GatherRoomScopedVoteCandidates(this, InstigatorCharacter, CandidateControllers);
	for (APlayerController* PlayerController : CandidateControllers)
	{
		ACharacter* Character = PlayerController ? Cast<ACharacter>(PlayerController->GetPawn()) : nullptr;
		AMyPlayerController* ClientEventBridge = Cast<AMyPlayerController>(PlayerController);
		if (!PlayerController || !Character || !ClientEventBridge || !IsParticipantEligible(PlayerController))
		{
			continue;
		}

		ClientEventBridge->SetActiveGroupEvent(this);
		Participants.Add(PlayerController);
	}

	if (Participants.Num() == 0)
	{
		return;
	}

	const URewardDefinition* EventRewardDefinition = GetRewardDefinition();
	const FText Title = EventRewardDefinition && !EventRewardDefinition->DisplayName.IsEmpty()
		? EventRewardDefinition->DisplayName
		: FText::FromString(TEXT("Judgment"));
	const int32 MaliceThreshold = ResolveMaliceThreshold();
	const FText Description = FText::Format(
		FText::FromString(TEXT("선택된 자에게 악의가 {0}개 이상이라면 그자는 죽습니다.")),
		FText::AsNumber(MaliceThreshold)
	);
	const float VoteDuration = static_cast<float>(ResolveVoteDurationSeconds());

	for (APlayerController* PlayerController : Participants)
	{
		if (AMyPlayerController* ClientEventBridge = Cast<AMyPlayerController>(PlayerController))
		{
			ClientEventBridge->OpenGroupEventVote(EventID, Title, Description, VoteDuration);
		}
	}

	GetWorld()->GetTimerManager().ClearTimer(VoteTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		VoteTimerHandle,
		this,
		&UGroupEventJudgment::ResolveVote,
		VoteDuration,
		false
	);
}

bool UGroupEventJudgment::SubmitVote(APlayerController* VotingPlayerController, int32 TargetPlayerId)
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
	for (APlayerController* Participant : Participants)
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

void UGroupEventJudgment::ResolveVote()
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
		FinishEvent(FText::FromString(TEXT("아무도 투표하지 않아 심판이 무효 처리되었습니다.")));
		return;
	}

	TMap<int32, int32> VoteCounts;
	for (const TPair<TObjectPtr<APlayerController>, int32>& VoteEntry : SubmittedVotes)
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
		FinishEvent(FText::FromString(TEXT("심판 대상을 결정하지 못했습니다.")));
		return;
	}

	const int32 SelectedPlayerId = TopPlayerIds[FMath::RandRange(0, TopPlayerIds.Num() - 1)];

	APlayerController* TargetPlayerController = nullptr;
	for (APlayerController* Participant : Participants)
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
		FinishEvent(FText::FromString(TEXT("심판 대상을 찾지 못했습니다.")));
		return;
	}

	const int32 MaliceThreshold = ResolveMaliceThreshold();
	int32 TargetMaliceCount = 0;
	bool bKilledByJudgment = false;

	if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetPlayerController->GetPawn()))
	{
		if (const UMaliceComponent* MaliceComponent = TargetCharacter->FindComponentByClass<UMaliceComponent>())
		{
			TargetMaliceCount = MaliceComponent->GetMaliceCount();
		}

		if (TargetMaliceCount >= MaliceThreshold)
		{
			if (AMyCharacter* TargetMyCharacter = Cast<AMyCharacter>(TargetCharacter))
			{
				TargetMyCharacter->ForceDeathByPersonalEvent();
				bKilledByJudgment = true;
			}
			else if (UCharacterHealthComponent* HealthComponent = TargetCharacter->FindComponentByClass<UCharacterHealthComponent>())
			{
				HealthComponent->ForceDeathByPersonalEvent();
				bKilledByJudgment = true;
			}
		}
	}

	FText ResultText;
	if (bKilledByJudgment)
	{
		ResultText = FText::Format(
			FText::FromString(TEXT("{0} 님이 선택되었고 악의가 {1}개 이상이라 죽었습니다.")),
			FText::FromString(ResolveJudgmentVotePlayerName(TargetPlayerController)),
			FText::AsNumber(MaliceThreshold)
		);
	}
	else if (TargetMaliceCount <= 1)
	{
		ResultText = FText::FromString(TEXT("아무일도 일어나지 않았습니다."));
	}
	else
	{
		ResultText = FText::Format(
			FText::FromString(TEXT("{0} 님이 선택되었습니다.")),
			FText::FromString(ResolveJudgmentVotePlayerName(TargetPlayerController))
		);
	}

	FinishEvent(ResultText);
}

void UGroupEventJudgment::FinishEvent(const FText& ResultText)
{
	for (APlayerController* Participant : Participants)
	{
		if (AMyPlayerController* ClientEventBridge = Cast<AMyPlayerController>(Participant))
		{
			ClientEventBridge->FinishGroupEventVote(EventID, ResultText);
			ClientEventBridge->SetActiveGroupEvent(nullptr);
		}
	}

	Participants.Reset();
	SubmittedVotes.Reset();
}

int32 UGroupEventJudgment::ResolveVoteDurationSeconds() const
{
	const UGroupEventJudgmentDefinition* EventDefinition =
		Cast<UGroupEventJudgmentDefinition>(const_cast<URewardDefinition*>(GetRewardDefinition()));
	if (!EventDefinition)
	{
		return 10;
	}

	return FMath::Max(1, FMath::RoundToInt(EventDefinition->Payload.VoteDurationSeconds));
}

int32 UGroupEventJudgment::ResolveMaliceThreshold() const
{
	const UGroupEventJudgmentDefinition* EventDefinition =
		Cast<UGroupEventJudgmentDefinition>(const_cast<URewardDefinition*>(GetRewardDefinition()));
	if (!EventDefinition)
	{
		return 2;
	}

	return FMath::Max(1, EventDefinition->Payload.MaliceThreshold);
}
