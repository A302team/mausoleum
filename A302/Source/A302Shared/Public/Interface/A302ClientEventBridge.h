#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "A302ClientEventBridge.generated.h"

class UBaseEvent;
class UBaseGroupEvent;

UINTERFACE()
class A302SHARED_API UA302ClientEventBridge : public UInterface
{
	GENERATED_BODY()
};

class A302SHARED_API IA302ClientEventBridge
{
	GENERATED_BODY()

public:
	virtual void SetActivePersonalEvent(UBaseEvent* Event) = 0;
	virtual void SetActiveGroupEvent(UBaseGroupEvent* Event) = 0;
	virtual void ShowPersonalEvent(FName EventID, const FText& Title, const FText& Description, const TArray<FText>& Choices) = 0;
	virtual void ShowInspectMaliceSelectionWidget() = 0;
	virtual void OpenGroupEventVote(FName EventID, const FText& EventTitle, const FText& EventDescription, float VoteDuration) = 0;
	virtual void FinishGroupEventVote(FName EventID, const FText& ResultText) = 0;
	virtual void ApplyConfiscationToLocalInventory() = 0;
	virtual void UpdateShieldCount(int32 ShieldCount) = 0;
	virtual void UpdateMaliceCount(int32 MaliceCount) = 0;
	virtual void UpdateItemTimer(float RemainingSeconds) = 0;
	virtual void SetItemTimerVisibleForClient(bool bVisible) = 0;
	virtual void ToggleVoiceChatCapture() = 0;
	virtual void ToggleInGameSettingMenu() = 0;
	virtual void ShowPublicMaliceAnnouncement(const FString& PlayerName, int32 MaliceCount) = 0;
};
