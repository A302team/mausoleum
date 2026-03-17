#include "Character/Components/PlayerEventComponent.h"

#include "Room/RoomScopeRules.h"
#include "Character/Components/ItemManagerComponent.h"
#include "Character/Components/PlayerHUDComponent.h"
#include "Character/MyPlayerController.h"
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

void UPlayerEventComponent::Client_ShowPersonalEvent_Implementation(FName EventID, const FText& Title, const FText& Description, const TArray<FText>& Choices)
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController)
	{
		return;
	}

	if (UPlayerHUDComponent* HUDComponent = OwnerController->GetPlayerHUDComponent())
	{
		HUDComponent->ShowPersonalEventUI(OwnerController->PersonalEventWidgetClass, EventID, Title, Description, Choices);
	}
}

void UPlayerEventComponent::Client_OpenGroupEventVote_Implementation(FName EventID, const FText& EventTitle, const FText& EventDescription, float VoteDuration)
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController)
	{
		return;
	}

	if (UPlayerHUDComponent* HUDComponent = OwnerController->GetPlayerHUDComponent())
	{
		HUDComponent->ShowGroupEventVoteUI(OwnerController->GroupEventVoteWidgetClass, EventID, EventTitle, EventDescription, VoteDuration);
	}
}

void UPlayerEventComponent::Client_FinishGroupEventVote_Implementation(FName EventID, const FText& ResultText)
{
	if (AMyPlayerController* OwnerController = GetOwnerController())
	{
		if (UPlayerHUDComponent* HUDComponent = OwnerController->GetPlayerHUDComponent())
		{
			HUDComponent->FinishGroupEventVoteUI(EventID, ResultText);
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

	if (ChoiceIndex == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Event] %s 거절됨."), *EventID.ToString());
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
			TargetEvent->OnEventResolvedMulti(ControlledCharacter, ChoiceIndex);
		}
	}

	ActivePersonalEvent = nullptr;
}

