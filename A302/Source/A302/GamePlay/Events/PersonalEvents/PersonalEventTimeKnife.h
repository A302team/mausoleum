#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"
#include "PersonalEventTimeKnife.generated.h"

class UItemDefinition;
class UPersonalEventTimeKnifeDefinition;
class URewardDefinition;

UCLASS(BlueprintType)
class A302_API UPersonalEventTimeKnife : public UBasePersonalEvent
{
	GENERATED_BODY()

public:
	virtual void ExecuteEvent_Implementation(AMyCharacter* InstigatorCharacter) override;
	void NotifyKillConfirmed();
	void CancelCountdown();
	virtual void BeginDestroy() override;

private:
	void HandleCountdownTick();
	void RefreshTimerUI() const;
	void StopCountdown(bool bHideTimer);
	UItemDefinition* ResolveGrantedKnifeDefinition(const URewardDefinition* SourceRewardDefinition, const UPersonalEventTimeKnifeDefinition* EventDefinition) const;

	UPROPERTY()
	TObjectPtr<AMyCharacter> OwnerCharacter = nullptr;

	FName GrantedItemId = NAME_None;
	float RemainingSeconds = 0.0f;
	bool bIsActive = false;
	FTimerHandle CountdownTimerHandle;
};
