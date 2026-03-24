#include "Character/Components/PlayerEventComponent.h"

#include "Room/RoomScopeRules.h"
#include "Character/Components/Inventory/ItemManagerComponent.h"
#include "Character/MyPlayerController.h"
#include "Character/MyCharacter.h"
#include "Interface/A302TimedKillEventBridge.h"
#include "Engine/World.h"
#include "GameFramework/HUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GamePlay/Events/BaseEvent.h"
#include "GamePlay/Events/GroupEvents/BaseGroupEvent.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"

namespace
{
	FText BuildTimedKnifeCountdownContext(float RemainingSeconds)
	{
		const int32 SafeSeconds = FMath::Max(0, FMath::CeilToInt(RemainingSeconds));
		return FText::FromString(FString::Printf(TEXT("피를 갈망하는 저주받은 검을 습득했습니다.\n%d초 내에 누군가를 공격하지 않으면 검 끝이 당신을 향할 것입니다."), SafeSeconds));
	}
}

UPlayerEventComponent::UPlayerEventComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

AMyPlayerController* UPlayerEventComponent::GetOwnerController() const
{
	if (AMyPlayerController* OwnerController = Cast<AMyPlayerController>(GetOwner()))
	{
		return OwnerController;
	}

	if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		return Cast<AMyPlayerController>(OwnerPawn->GetController());
	}

	return nullptr;
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
	PauseTimedKillCountdownForVote();
	Client_OpenGroupEventVote(EventID, EventTitle, EventDescription, VoteDuration);
}

void UPlayerEventComponent::FinishGroupEventVote(FName EventID, const FText& ResultText)
{
	ResumeTimedKillCountdownForVote();
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
	if (ActiveTimedKnifeEventObject)
	{
		if (IA302TimedKillEventBridge* TimedKillBridge = Cast<IA302TimedKillEventBridge>(ActiveTimedKnifeEventObject))
		{
			TimedKillBridge->NotifyTimedKillConfirmed();
		}
		return;
	}

	if (!ActiveTimedKnifeItemId.IsNone() && TimedKnifeRemainingSeconds > 0.0f)
	{
		StopTimedKnifeCountdown(true, true);
	}
}

void UPlayerEventComponent::NotifyTimedKnifeAttackSucceeded()
{
	if (IA302TimedKillEventBridge* TimedKillBridge = Cast<IA302TimedKillEventBridge>(ActiveTimedKnifeEventObject))
	{
		TimedKillBridge->NotifyTimedKillConfirmed();
		return;
	}

	if (!ActiveTimedKnifeItemId.IsNone() && TimedKnifeRemainingSeconds > 0.0f)
	{
		StopTimedKnifeCountdown(true, true);
	}
}

void UPlayerEventComponent::RegisterTimedKillEvent(UObject* EventInstance)
{
	if (IA302TimedKillEventBridge* ExistingTimedKillBridge = Cast<IA302TimedKillEventBridge>(ActiveTimedKnifeEventObject);
		ExistingTimedKillBridge && ActiveTimedKnifeEventObject != EventInstance)
	{
		ExistingTimedKillBridge->CancelTimedKillCountdown();
	}

	StopTimedKnifeCountdown(false, false);
	ActiveTimedKnifeEventObject = EventInstance;
}

void UPlayerEventComponent::ClearTimedKillEvent(UObject* EventInstance)
{
	if (!EventInstance || ActiveTimedKnifeEventObject == EventInstance)
	{
		ActiveTimedKnifeEventObject = nullptr;
	}
}

void UPlayerEventComponent::PauseActiveTimedKillCountdown()
{
	if (IA302TimedKillEventBridge* TimedKillBridge = Cast<IA302TimedKillEventBridge>(ActiveTimedKnifeEventObject))
	{
		TimedKillBridge->PauseTimedKillCountdown();
	}
}

void UPlayerEventComponent::ResumeActiveTimedKillCountdown()
{
	if (IA302TimedKillEventBridge* TimedKillBridge = Cast<IA302TimedKillEventBridge>(ActiveTimedKnifeEventObject))
	{
		TimedKillBridge->ResumeTimedKillCountdown();
	}
}

void UPlayerEventComponent::PauseTimedKillCountdownForVote()
{
	if (GroupVotePauseDepth == 0)
	{
		PauseActiveTimedKillCountdown();

		if (AMyPlayerController* OwnerController = GetOwnerController())
		{
			if (APawn* ControlledPawn = OwnerController->GetPawn())
			{
				if (UPlayerEventComponent* PawnEventComponent = ControlledPawn->FindComponentByClass<UPlayerEventComponent>())
				{
					if (PawnEventComponent != this)
					{
						PawnEventComponent->PauseActiveTimedKillCountdown();
					}
				}
			}
		}
	}

	++GroupVotePauseDepth;
}

void UPlayerEventComponent::ResumeTimedKillCountdownForVote()
{
	if (GroupVotePauseDepth <= 0)
	{
		return;
	}

	--GroupVotePauseDepth;
	if (GroupVotePauseDepth > 0)
	{
		return;
	}

	ResumeActiveTimedKillCountdown();

	if (AMyPlayerController* OwnerController = GetOwnerController())
	{
		if (APawn* ControlledPawn = OwnerController->GetPawn())
		{
			if (UPlayerEventComponent* PawnEventComponent = ControlledPawn->FindComponentByClass<UPlayerEventComponent>())
			{
				if (PawnEventComponent != this)
				{
					PawnEventComponent->ResumeActiveTimedKillCountdown();
				}
			}
		}
	}
}

void UPlayerEventComponent::SetTimedKnifeAttackInProgress(bool bInProgress)
{
	bTimedKnifeAttackInProgress = bInProgress;
}

void UPlayerEventComponent::StartTimedKnifeCountdown(float DurationSeconds, const FName& ItemId, const FText& Title)
{
	AMyCharacter* OwnerCharacter = Cast<AMyCharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}

	if (IA302TimedKillEventBridge* ExistingTimedKillBridge = Cast<IA302TimedKillEventBridge>(ActiveTimedKnifeEventObject))
	{
		ExistingTimedKillBridge->CancelTimedKillCountdown();
	}
	ActiveTimedKnifeEventObject = nullptr;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimedKnifeCountdownTimerHandle);
	}

	TimedKnifeRemainingSeconds = FMath::Max(1.0f, DurationSeconds);
	ActiveTimedKnifeItemId = ItemId;
	ActiveTimedKnifeTitle = Title.IsEmpty() ? FText::FromString(TEXT("저주받은 검")) : Title;
	bTimedKnifeAttackInProgress = false;

	if (AMyPlayerController* OwnerController = GetOwnerController())
	{
		OwnerController->UpdateItemTimer(TimedKnifeRemainingSeconds);
		OwnerController->SetItemTimerVisibleForClient(true);
		OwnerController->Client_ShowTitleCard(ActiveTimedKnifeTitle, BuildTimedKnifeCountdownContext(TimedKnifeRemainingSeconds), 1.5f);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			TimedKnifeCountdownTimerHandle,
			this,
			&UPlayerEventComponent::HandleTimedKnifeCountdownTick,
			1.0f,
			true
		);
	}
}

void UPlayerEventComponent::StopTimedKnifeCountdown(bool bHideTimer, bool bConsumeItem)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimedKnifeCountdownTimerHandle);
	}

	AMyCharacter* OwnerCharacter = Cast<AMyCharacter>(GetOwner());
	if (OwnerCharacter && OwnerCharacter->HasAuthority() && bConsumeItem && !ActiveTimedKnifeItemId.IsNone())
	{
		if (UItemManagerComponent* ItemManager = OwnerCharacter->FindComponentByClass<UItemManagerComponent>())
		{
			int32 RemovedSlotIndex = INDEX_NONE;
			ItemManager->RemoveFirstItemByItemId(ActiveTimedKnifeItemId, RemovedSlotIndex);
		}
	}

	if (bHideTimer)
	{
		if (AMyPlayerController* OwnerController = GetOwnerController())
		{
			OwnerController->SetItemTimerVisibleForClient(false);
		}
	}

	TimedKnifeRemainingSeconds = 0.0f;
	ActiveTimedKnifeItemId = NAME_None;
	ActiveTimedKnifeTitle = FText::GetEmpty();
	bTimedKnifeAttackInProgress = false;
}

void UPlayerEventComponent::HandleTimedKnifeCountdownTick()
{
	AMyCharacter* OwnerCharacter = Cast<AMyCharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		StopTimedKnifeCountdown(true, false);
		return;
	}

	TimedKnifeRemainingSeconds = FMath::Max(0.0f, TimedKnifeRemainingSeconds - 1.0f);
	if (AMyPlayerController* OwnerController = GetOwnerController())
	{
		OwnerController->UpdateItemTimer(TimedKnifeRemainingSeconds);
		OwnerController->SetItemTimerVisibleForClient(true);
		OwnerController->Client_ShowTitleCard(ActiveTimedKnifeTitle, BuildTimedKnifeCountdownContext(TimedKnifeRemainingSeconds), 1.5f);
	}

	if (TimedKnifeRemainingSeconds > 0.0f)
	{
		return;
	}

	OwnerCharacter->ForceDeathByPersonalEvent();
	StopTimedKnifeCountdown(true, true);
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
