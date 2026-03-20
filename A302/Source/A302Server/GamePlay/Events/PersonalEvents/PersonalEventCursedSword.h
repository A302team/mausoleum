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
	virtual void OnEventResolved(ACharacter* InstigatorCharacter, int32 ChoiceIndex) override;
	void NotifyTimedKillConfirmed();
	void CancelTimedKillCountdown();
	virtual void BeginDestroy() override;

private:
	bool TryGrantCursedSwordToPreferredSlot(ACharacter* InstigatorCharacter, UItemDefinition* GrantedCursedSwordDefinition, int32& OutAddedSlotIndex) const;
	void HandleCountdownTick();
	void RefreshTimerUI() const;
	void StopCountdown(bool bHideTimer);
	UItemDefinition* ResolveGrantedCursedSwordDefinition(const URewardDefinition* SourceRewardDefinition, const UPersonalEventCursedSwordDefinition* EventDefinition) const;

	UPROPERTY()
	TObjectPtr<ACharacter> OwnerCharacter = nullptr;

	FName GrantedItemId = NAME_None;
	int32 GrantedSlotIndex = INDEX_NONE;
	float RemainingSeconds = 0.0f;
	bool bIsActive = false;
	FTimerHandle CountdownTimerHandle;
};
