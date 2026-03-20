#include "Character/Components/PlayerEventComponent.h"

#include "Room/RoomScopeRules.h"
#include "Character/Components/Inventory/ItemManagerComponent.h"
#include "Character/MyPlayerController.h"
#include "Character/MyCharacter.h"
#include "Interface/A302TimedKillEventBridge.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "GamePlay/Events/BaseEvent.h"
#include "GamePlay/Events/GroupEvents/BaseGroupEvent.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"

UPlayerEventComponent::UPlayerEventComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

AMyPlayerController* UPlayerEventComponent::GetOwnerController() const
{
	return Cast<AMyPlayerController>(GetOwner());
}

void UPlayerEventComponent::SetActivePersonalEvent(UBaseEvent* Event)
{
	ActivePersonalEvent = Event;
}

void UPlayerEventComponent::SetActiveGroupEvent(UBaseGroupEvent* Event)
{
	ActiveGroupEvent = Event;
}

void UPlayerEventComponent::ShowPersonalEvent(FName EventID, const FText& Title, const FText& Description, const TArray<FText>& Choices)
{
	Client_ShowPersonalEvent(EventID, Title, Description, Choices);
}

void UPlayerEventComponent::ShowInspectMaliceSelectionWidget()
{
	Client_ShowInspectMaliceSelectionWidget();
}

void UPlayerEventComponent::ShowInspectMaliceSelectionWidgetWithConfig(float SelectionTimeoutSeconds, float ResultDisplaySeconds)
{
	Client_ShowInspectMaliceSelectionWidgetWithConfig(SelectionTimeoutSeconds, ResultDisplaySeconds);
}

void UPlayerEventComponent::OpenGroupEventVote(FName EventID, const FText& EventTitle, const FText& EventDescription, float VoteDuration)
{
	Client_OpenGroupEventVote(EventID, EventTitle, EventDescription, VoteDuration);
}

void UPlayerEventComponent::FinishGroupEventVote(FName EventID, const FText& ResultText)
{
	Client_FinishGroupEventVote(EventID, ResultText);
}

void UPlayerEventComponent::ApplyConfiscationToLocalInventory()
{
	Client_ApplyConfiscationToLocalInventory();
}

void UPlayerEventComponent::RequestResolvePersonalEvent(FName EventID, int32 ChoiceIndex)
{
	Server_ResolvePersonalEvent(EventID, ChoiceIndex);
}

void UPlayerEventComponent::RequestSubmitGroupVote(FName EventID, int32 TargetPlayerId)
{
	Server_SubmitGroupVote(EventID, TargetPlayerId);
}

void UPlayerEventComponent::NotifyKilledCharacter()
{
	if (ActiveTimedKnifeEventObject && bTimedKnifeAttackInProgress)
	{
		if (IA302TimedKillEventBridge* TimedKillBridge = Cast<IA302TimedKillEventBridge>(ActiveTimedKnifeEventObject))
		{
			TimedKillBridge->NotifyTimedKillConfirmed();
		}
	}
}

void UPlayerEventComponent::NotifyTimedKnifeAttackSucceeded()
{
	if (IA302TimedKillEventBridge* TimedKillBridge = Cast<IA302TimedKillEventBridge>(ActiveTimedKnifeEventObject))
	{
		TimedKillBridge->NotifyTimedKillConfirmed();
	}
}

void UPlayerEventComponent::RegisterTimedKillEvent(UObject* EventInstance)
{
	if (IA302TimedKillEventBridge* ExistingTimedKillBridge = Cast<IA302TimedKillEventBridge>(ActiveTimedKnifeEventObject);
		ExistingTimedKillBridge && ActiveTimedKnifeEventObject != EventInstance)
	{
		ExistingTimedKillBridge->CancelTimedKillCountdown();
	}

	ActiveTimedKnifeEventObject = EventInstance;
}

void UPlayerEventComponent::ClearTimedKillEvent(UObject* EventInstance)
{
	if (!EventInstance || ActiveTimedKnifeEventObject == EventInstance)
	{
		ActiveTimedKnifeEventObject = nullptr;
	}
}

void UPlayerEventComponent::SetTimedKnifeAttackInProgress(bool bInProgress)
{
	bTimedKnifeAttackInProgress = bInProgress;
}

void UPlayerEventComponent::Client_ShowPersonalEvent_Implementation(FName EventID, const FText& Title, const FText& Description, const TArray<FText>& Choices)
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController)
	{
		return;
	}

	if (AHUD* GameHUD = OwnerController->GetHUD())
	{
		if (UFunction* Func = GameHUD->FindFunction(TEXT("ShowPersonalEvent")))
		{
			struct FParams { FName Id; FText T; FText D; TArray<FText> C; };
			FParams Params { EventID, Title, Description, Choices };
			GameHUD->ProcessEvent(Func, &Params);
		}
	}
}

void UPlayerEventComponent::Client_ShowInspectMaliceSelectionWidget_Implementation()
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController)
	{
		return;
	}

	OwnerController->Client_ShowInspectMaliceSelectionWidget();
}

void UPlayerEventComponent::Client_ShowInspectMaliceSelectionWidgetWithConfig_Implementation(float SelectionTimeoutSeconds, float ResultDisplaySeconds)
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController)
	{
		return;
	}

	OwnerController->Client_ShowInspectMaliceSelectionWidgetWithConfig(SelectionTimeoutSeconds, ResultDisplaySeconds);
}

void UPlayerEventComponent::Client_OpenGroupEventVote_Implementation(FName EventID, const FText& EventTitle, const FText& EventDescription, float VoteDuration)
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController)
	{
		return;
	}

	if (AHUD* GameHUD = OwnerController->GetHUD())
	{
		if (UFunction* Func = GameHUD->FindFunction(TEXT("ShowGroupEventVote")))
		{
			struct FParams { FName Id; FText T; FText D; float V; };
			FParams Params { EventID, EventTitle, EventDescription, VoteDuration };
			GameHUD->ProcessEvent(Func, &Params);
		}
	}
}

void UPlayerEventComponent::Client_FinishGroupEventVote_Implementation(FName EventID, const FText& ResultText)
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController)
	{
		return;
	}

	if (AHUD* GameHUD = OwnerController->GetHUD())
	{
		if (UFunction* Func = GameHUD->FindFunction(TEXT("FinishGroupEventVoteUI")))
		{
			struct FParams { FName Id; FText R; };
			FParams Params { EventID, ResultText };
			GameHUD->ProcessEvent(Func, &Params);
		}
	}
}

void UPlayerEventComponent::Client_ApplyConfiscationToLocalInventory_Implementation()
{
	ACharacter* ControlledCharacter = Cast<ACharacter>(GetOwnerController() ? GetOwnerController()->GetPawn() : nullptr);
	if (!ControlledCharacter)
	{
		return;
	}

	if (UItemManagerComponent* ItemManager = ControlledCharacter->FindComponentByClass<UItemManagerComponent>())
	{
		ItemManager->RemoveAllItems();
	}
}

void UPlayerEventComponent::Server_SubmitGroupVote_Implementation(FName EventID, int32 TargetPlayerId)
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController || !ActiveGroupEvent || ActiveGroupEvent->EventID != EventID)
	{
		return;
	}

	if (!A302RoomScope::DoesEventBelongToPlayerRoom(OwnerController->PlayerState, ActiveGroupEvent))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GroupEventVote] Rejected vote from another logical room. player=%s event=%s"), *GetNameSafe(OwnerController), *EventID.ToString());
		return;
	}

	ActiveGroupEvent->SubmitVote(OwnerController, TargetPlayerId);
}

void UPlayerEventComponent::Server_ResolvePersonalEvent_Implementation(FName EventID, int32 ChoiceIndex)
{
	AMyPlayerController* OwnerController = GetOwnerController();
	ACharacter* ControlledCharacter = Cast<ACharacter>(OwnerController ? OwnerController->GetPawn() : nullptr);
	if (!OwnerController || !ControlledCharacter)
	{
		return;
	}

	if (!ActivePersonalEvent)
	{
		return;
	}

	if (UBasePersonalEvent* TargetEvent = Cast<UBasePersonalEvent>(ActivePersonalEvent))
	{
		if (!A302RoomScope::DoesEventBelongToPlayerRoom(OwnerController->PlayerState, TargetEvent))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Event] Ignored resolve from another logical room. event=%s"), *EventID.ToString());
			return;
		}

		if (TargetEvent->EventID == EventID)
		{
			TargetEvent->OnEventResolved(ControlledCharacter, ChoiceIndex);
		}
	}

	ActivePersonalEvent = nullptr;
}
