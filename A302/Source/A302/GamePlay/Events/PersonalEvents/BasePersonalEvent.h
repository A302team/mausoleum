#pragma once

#include "CoreMinimal.h"
#include "GamePlay/Events/BaseEvent.h"
#include "BasePersonalEvent.generated.h"

class AActor;
class URewardDefinition;

UCLASS(BlueprintType, Abstract)
class A302_API UBasePersonalEvent : public UBaseEvent
{
	GENERATED_BODY()

public:
	void InitializeContext(const URewardDefinition* InRewardDefinition, AActor* InSourceActor);
	virtual void OnEventResolved(class AMyCharacter* InstigatorCharacter, bool bIsConfirmed) {}
	virtual void OnEventResolvedMulti(class AMyCharacter* InstigatorCharacter, int32 ChoiceIndex)
	{
		// 복수 선택지 대응(0은 취소)
		OnEventResolved(InstigatorCharacter, ChoiceIndex > 0);
	}

protected:
	const URewardDefinition* GetRewardDefinition() const { return RewardDefinition; }
	AActor* GetSourceActor() const { return SourceActor.Get(); }

private:
	const URewardDefinition* RewardDefinition = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> SourceActor = nullptr;
};
