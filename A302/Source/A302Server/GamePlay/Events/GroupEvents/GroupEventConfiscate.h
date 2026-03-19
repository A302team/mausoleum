#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GamePlay/Events/GroupEvents/BaseGroupEvent.h"
#include "GroupEventConfiscate.generated.h"

UCLASS()
class A302SERVER_API UGroupEventConfiscate : public UBaseGroupEvent
{
	GENERATED_BODY()

public:
	virtual void ExecuteEvent_Implementation(class ACharacter* InstigatorCharacter) override;
	virtual bool SubmitVote(APlayerController* VotingPlayerController, int32 TargetPlayerId) override;

private:
	void ResolveVote();
	void FinishEvent(const FText& ResultText);
	int32 ResolveVoteDurationSeconds() const;

	TArray<TObjectPtr<APlayerController>> Participants;
	TMap<TObjectPtr<APlayerController>, int32> SubmittedVotes;
	FTimerHandle VoteTimerHandle;
	bool bResolved = false;
};
