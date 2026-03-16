#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Events/BaseEvent.h"
#include "BaseGroupEvent.generated.h"

class AActor;
class AMyPlayerController;
class URewardDefinition;

UCLASS(BlueprintType, Abstract)
class A302_API UBaseGroupEvent : public UBaseEvent
{
	GENERATED_BODY()

public:
	void InitializeContext(const URewardDefinition* InRewardDefinition, AActor* InSourceActor);
	virtual bool SubmitVote(AMyPlayerController* VotingPlayerController, int32 TargetPlayerId) { return false; }

protected:
	const URewardDefinition* GetRewardDefinition() const { return RewardDefinition; }
	AActor* GetSourceActor() const { return SourceActor.Get(); }

private:
	const URewardDefinition* RewardDefinition = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> SourceActor = nullptr;
};
