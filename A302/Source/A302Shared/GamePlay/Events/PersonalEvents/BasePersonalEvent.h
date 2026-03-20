#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Events/BaseEvent.h"
#include "BasePersonalEvent.generated.h"

class AActor;
class ACharacter;
class URewardDefinition;

UCLASS(BlueprintType, Abstract)
class A302SHARED_API UBasePersonalEvent : public UBaseEvent
{
	GENERATED_BODY()

public:
	void InitializeContext(const URewardDefinition* InRewardDefinition, AActor* InSourceActor);
	virtual void OnEventResolved(ACharacter* InstigatorCharacter, int32 ChoiceIndex) {}

protected:
	const URewardDefinition* GetRewardDefinition() const { return RewardDefinition; }
	AActor* GetSourceActor() const { return SourceActor.Get(); }

private:
	const URewardDefinition* RewardDefinition = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> SourceActor = nullptr;
};
