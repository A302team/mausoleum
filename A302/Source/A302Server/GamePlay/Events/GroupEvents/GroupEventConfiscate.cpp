#include "GamePlay/Events/GroupEvents/GroupEventConfiscate.h"

#include "Character/MyPlayerController.h"
#include "Character/Components/Inventory/ItemManagerComponent.h"
#include "Engine/World.h"
#include "GameData/Events/GroupEvents/GroupEventConfiscateDefinition.h"
#include "GameData/RewardDefinition.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"

namespace
{
	constexpr float ConfiscationExecuteDelaySeconds = 1.0f;

	FString ResolveConfiscateVotePlayerName(const APlayerController* PlayerController)
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
}

void UGroupEventConfiscate::ExecuteEvent_Implementation(ACharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter || !InstigatorCharacter->HasAuthority() || !GetWorld() || bResolved)
	{
		return;
	}

	Participants.Reset();
	SubmittedVotes.Reset();
	InitializeRuntimeContext(InstigatorCharacter);

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
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
		: FText::FromString(TEXT("Vote"));
	const FText Description = EventRewardDefinition && !EventRewardDefinition->Description.IsEmpty()
		? EventRewardDefinition->Description
		: FText::FromString(TEXT("가장 많은 선택을 받은 유저의 모든 아이템을 몰수합니다."));
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
		&UGroupEventConfiscate::ResolveVote,
		VoteDuration,
		false
	);
}

bool UGroupEventConfiscate::SubmitVote(APlayerController* VotingPlayerController, int32 TargetPlayerId)
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
		FinishEvent(FText::FromString(TEXT("아무도 투표하지 않아 몰수가 진행되지 않습니다.")));
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
		FinishEvent(FText::FromString(TEXT("몰수 대상을 결정하지 못했습니다.")));
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
		FinishEvent(FText::FromString(TEXT("몰수 대상을 찾지 못했습니다.")));
		return;
	}

	const FString TargetName = ResolveConfiscateVotePlayerName(TargetPlayerController);
	const FText ResultText = FText::FromString(
		FString::Printf(
			TEXT("%s님이 선택되었습니다.\n%s님의 모든 아이템을 몰수합니다."),
			*TargetName,
			*TargetName
		)
	);

	FinishEvent(ResultText);

	if (UWorld* World = GetWorld())
	{
		const TWeakObjectPtr<APlayerController> WeakTargetPlayerController = TargetPlayerController;
		FTimerDelegate DelayedConfiscationDelegate;
		DelayedConfiscationDelegate.BindLambda([WeakTargetPlayerController]()
		{
			APlayerController* DelayedTargetPlayerController = WeakTargetPlayerController.Get();
			if (!DelayedTargetPlayerController)
			{
				return;
			}

			if (ACharacter* TargetCharacter = Cast<ACharacter>(DelayedTargetPlayerController->GetPawn()))
			{
				if (UItemManagerComponent* ItemManager = TargetCharacter->FindComponentByClass<UItemManagerComponent>())
				{
					ItemManager->RemoveAllItems();
				}
			}

			if (AMyPlayerController* ClientEventBridge = Cast<AMyPlayerController>(DelayedTargetPlayerController))
			{
				ClientEventBridge->ApplyConfiscationToLocalInventory();
			}
		});

		FTimerHandle DelayedConfiscationHandle;
		World->GetTimerManager().SetTimer(
			DelayedConfiscationHandle,
			DelayedConfiscationDelegate,
			ConfiscationExecuteDelaySeconds,
			false
		);
	}
}

void UGroupEventConfiscate::FinishEvent(const FText& ResultText)
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
