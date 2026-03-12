#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"
#include "PersonalEventCursedSword.generated.h"

class UItemDefinition;
class UPersonalEventCursedSwordDefinition;
class URewardDefinition;

UCLASS(BlueprintType)
class A302_API UPersonalEventCursedSword : public UBasePersonalEvent
{
	GENERATED_BODY()

public:
	virtual void ExecuteEvent_Implementation(AMyCharacter* InstigatorCharacter) override;
	virtual void OnEventResolved(AMyCharacter* InstigatorCharacter, bool bIsConfirmed) override;
	void NotifyKillConfirmed();
	void CancelCountdown();
	virtual void BeginDestroy() override;

private:
	void HandleCountdownTick();
	void RefreshTimerUI() const;
	void StopCountdown(bool bHideTimer);
	UItemDefinition* ResolveGrantedKnifeDefinition(const URewardDefinition* SourceRewardDefinition, const UPersonalEventCursedSwordDefinition* EventDefinition) const;

	UPROPERTY()
	TObjectPtr<AMyCharacter> OwnerCharacter = nullptr;

	FName GrantedItemId = NAME_None;
	float RemainingSeconds = 0.0f;
	bool bIsActive = false;
	FTimerHandle CountdownTimerHandle;
};
