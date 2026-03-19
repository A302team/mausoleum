#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Events/BaseEvent.h"
#include "BaseGroupEvent.generated.h"

class AActor;
class APlayerController;
class URewardDefinition;

UCLASS(BlueprintType, Abstract)
class A302SHARED_API UBaseGroupEvent : public UBaseEvent
{
	GENERATED_BODY()

public:
	void InitializeContext(const URewardDefinition* InRewardDefinition, AActor* InSourceActor);
	virtual bool SubmitVote(APlayerController* VotingPlayerController, int32 TargetPlayerId) { return false; }

protected:
	const URewardDefinition* GetRewardDefinition() const { return RewardDefinition; }
	AActor* GetSourceActor() const { return SourceActor.Get(); }
	bool IsParticipantEligible(const APlayerController* PlayerController) const;

private:
	const URewardDefinition* RewardDefinition = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> SourceActor = nullptr;
};
