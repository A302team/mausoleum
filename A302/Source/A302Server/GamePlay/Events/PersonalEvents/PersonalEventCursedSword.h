#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"
#include "Interface/A302TimedKillEventBridge.h"
#include "PersonalEventCursedSword.generated.h"

class UItemDefinition;
class UPersonalEventCursedSwordDefinition;
class URewardDefinition;
class ACharacter;

UCLASS(BlueprintType)
class A302SERVER_API UPersonalEventCursedSword : public UBasePersonalEvent, public IA302TimedKillEventBridge
{
	GENERATED_BODY()

public:
	virtual void ExecuteEvent_Implementation(ACharacter* InstigatorCharacter) override;
	virtual void OnEventResolved(ACharacter* InstigatorCharacter, bool bIsConfirmed) override;
	void NotifyTimedKillConfirmed();
	void CancelTimedKillCountdown();
	virtual void BeginDestroy() override;

private:
	void HandleCountdownTick();
	void RefreshTimerUI() const;
	void StopCountdown(bool bHideTimer);
	UItemDefinition* ResolveGrantedKnifeDefinition(const URewardDefinition* SourceRewardDefinition, const UPersonalEventCursedSwordDefinition* EventDefinition) const;

	UPROPERTY()
	TObjectPtr<ACharacter> OwnerCharacter = nullptr;

	FName GrantedItemId = NAME_None;
	float RemainingSeconds = 0.0f;
	bool bIsActive = false;
	FTimerHandle CountdownTimerHandle;
};
