#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Events/GroupEvents/BaseGroupEvent.h"
#include "GroupEventConfiscate.generated.h"

class AMyPlayerController;

UCLASS()
class A302_API UGroupEventConfiscate : public UBaseGroupEvent
{
	GENERATED_BODY()

public:
	virtual void ExecuteEvent_Implementation(class AMyCharacter* InstigatorCharacter) override;
	virtual bool SubmitVote(AMyPlayerController* VotingPlayerController, int32 TargetPlayerId) override;

private:
	void ResolveVote();
	void FinishEvent(const FText& ResultText);
	int32 ResolveVoteDurationSeconds() const;

	TArray<TObjectPtr<AMyPlayerController>> Participants;
	TMap<TObjectPtr<AMyPlayerController>, int32> SubmittedVotes;
	FTimerHandle VoteTimerHandle;
	bool bResolved = false;
};
