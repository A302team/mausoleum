#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Events/BaseEvent.h"
#include "BasePersonalEvent.generated.h"

class AActor;
class UItemDefinition;

UCLASS(BlueprintType, Abstract)
class A302_API UBasePersonalEvent : public UBaseEvent
{
	GENERATED_BODY()

public:
	void InitializeContext(const UItemDefinition* InRewardDefinition, AActor* InSourceActor);

protected:
	const UItemDefinition* GetRewardDefinition() const { return RewardDefinition; }
	AActor* GetSourceActor() const { return SourceActor.Get(); }

private:
	const UItemDefinition* RewardDefinition = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> SourceActor = nullptr;
};
