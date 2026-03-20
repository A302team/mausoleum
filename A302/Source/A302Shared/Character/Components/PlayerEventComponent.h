#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PlayerEventComponent.generated.h"

class AMyPlayerController;
class UBaseEvent;
class UBaseGroupEvent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302SHARED_API UPlayerEventComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerEventComponent();

	void SetActivePersonalEvent(UBaseEvent* Event);
	void SetActiveGroupEvent(UBaseGroupEvent* Event);
	void ShowPersonalEvent(FName EventID, const FText& Title, const FText& Description, const TArray<FText>& Choices);
	void ShowInspectMaliceSelectionWidget();
	void ShowInspectMaliceSelectionWidgetWithConfig(float SelectionTimeoutSeconds, float ResultDisplaySeconds);
	void OpenGroupEventVote(FName EventID, const FText& EventTitle, const FText& EventDescription, float VoteDuration);
	void FinishGroupEventVote(FName EventID, const FText& ResultText);
	void ApplyConfiscationToLocalInventory();
	void RequestResolvePersonalEvent(FName EventID, int32 ChoiceIndex);
	void RequestSubmitGroupVote(FName EventID, int32 TargetPlayerId);

	// Timed Kill Logic (Migrated from MyCharacter)
	void NotifyKilledCharacter();
	void NotifyTimedKnifeAttackSucceeded();
	void RegisterTimedKillEvent(UObject* EventInstance);
	void ClearTimedKillEvent(UObject* EventInstance);
	void SetTimedKnifeAttackInProgress(bool bInProgress);
	bool IsTimedKnifeAttackInProgress() const { return bTimedKnifeAttackInProgress; }

	UFUNCTION(Client, Reliable)
	void Client_ShowPersonalEvent(FName EventID, const FText& Title, const FText& Description, const TArray<FText>& Choices);

	UFUNCTION(Client, Reliable)
	void Client_ShowInspectMaliceSelectionWidget();

	UFUNCTION(Client, Reliable)
	void Client_ShowInspectMaliceSelectionWidgetWithConfig(float SelectionTimeoutSeconds, float ResultDisplaySeconds);

	UFUNCTION(Client, Reliable)
	void Client_OpenGroupEventVote(FName EventID, const FText& EventTitle, const FText& EventDescription, float VoteDuration);

	UFUNCTION(Client, Reliable)
	void Client_FinishGroupEventVote(FName EventID, const FText& ResultText);

	UFUNCTION(Client, Reliable)
	void Client_ApplyConfiscationToLocalInventory();

	UFUNCTION(Server, Reliable)
	void Server_ResolvePersonalEvent(FName EventID, int32 ChoiceIndex);

	UFUNCTION(Server, Reliable)
	void Server_SubmitGroupVote(FName EventID, int32 TargetPlayerId);

private:
	AMyPlayerController* GetOwnerController() const;

	UPROPERTY()
	TObjectPtr<UBaseEvent> ActivePersonalEvent;

	UPROPERTY()
	TObjectPtr<UBaseGroupEvent> ActiveGroupEvent;

	UPROPERTY()
	TObjectPtr<UObject> ActiveTimedKnifeEventObject = nullptr;

	bool bTimedKnifeAttackInProgress = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Timed", meta = (AllowPrivateAccess = "true"))
	float TimedKnifeRemainingSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Timed", meta = (AllowPrivateAccess = "true"))
	FName ActiveTimedKnifeItemId = NAME_None;
};
