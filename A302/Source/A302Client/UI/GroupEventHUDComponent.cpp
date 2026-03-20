#include "UI/GroupEventHUDComponent.h"

#include "Character/Components/PlayerEventComponent.h"
#include "Character/MyPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerState.h"
#include "GameMode/A302PlayerState.h"
#include "Room/RoomScopeRules.h"
#include "UI/VoteClickableUserWidget.h"

namespace
{
	constexpr int32 GroupEventVoteSlotCount = 6;
	constexpr int32 GroupEventMaxNicknameUiLen = 7;

	FString ClampGroupEventNicknameForUi(const FString& Name)
	{
		return Name.Len() > GroupEventMaxNicknameUiLen ? Name.Left(GroupEventMaxNicknameUiLen) : Name;
	}
}

UGroupEventHUDComponent::UGroupEventHUDComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

AMyPlayerController* UGroupEventHUDComponent::GetOwnerController() const
{
	if (const AHUD* OwningHUD = Cast<AHUD>(GetOwner()))
	{
		return Cast<AMyPlayerController>(OwningHUD->PlayerOwner);
	}
	return Cast<AMyPlayerController>(GetOwner());
}

void UGroupEventHUDComponent::InitializeGroupEventVoteWidget(TSubclassOf<UUserWidget> GroupEventVoteWidgetClass)
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController || !GroupEventVoteWidgetClass)
	{
		return;
	}

	if (!GroupEventVoteWidgetInstance)
	{
		GroupEventVoteWidgetInstance = CreateWidget<UUserWidget>(OwnerController, GroupEventVoteWidgetClass);
		if (!GroupEventVoteWidgetInstance)
		{
			return;
		}

		GroupEventVoteWidgetInstance->AddToViewport(125);
		GroupEventVoteWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}

	GroupEventVoteTitleText = FindGroupEventVoteText(TEXT("GroupEventVoteTitle"));
	GroupEventVoteDescriptionText = FindGroupEventVoteText(TEXT("GroupEventVoteDes"));
	GroupEventVoteTimerText = FindGroupEventVoteText(TEXT("VoteTimer"));

	VoteUserSlotWidgets.Reset();
	for (int32 SlotIndex = 0; SlotIndex < GroupEventVoteSlotCount; ++SlotIndex)
	{
		if (UVoteClickableUserWidget* VoteSlotWidget = FindVoteUserSlot(SlotIndex))
		{
			VoteUserSlotWidgets.Add(VoteSlotWidget);
			VoteSlotWidget->OnVoteClickableUserSelected.RemoveAll(this);
			VoteSlotWidget->OnVoteClickableUserSelected.AddUObject(this, &UGroupEventHUDComponent::HandleLocalGroupVoteSelection);
		}
	}
}

void UGroupEventHUDComponent::ShowGroupEventVoteUI(TSubclassOf<UUserWidget> GroupEventVoteWidgetClass, FName EventID, const FText& EventTitle, const FText& EventDescription, float VoteDuration)
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController)
	{
		return;
	}

	InitializeGroupEventVoteWidget(GroupEventVoteWidgetClass);
	if (!GroupEventVoteWidgetInstance)
	{
		return;
	}

	OwnerController->FlushPressedKeys();
	ActiveGroupVoteEventID = EventID;
	GroupEventVoteRemainingSeconds = FMath::Max(1, FMath::CeilToInt(VoteDuration));
	bHasSubmittedGroupVote = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GroupEventVoteCountdownHandle);
		World->GetTimerManager().ClearTimer(GroupEventVoteCloseHandle);
	}

	if (GroupEventVoteTitleText)
	{
		GroupEventVoteTitleText->SetText(EventTitle);
	}

	if (GroupEventVoteDescriptionText)
	{
		GroupEventVoteDescriptionText->SetText(EventDescription);
	}

	PopulateGroupEventVoteCandidates();
	UpdateGroupEventVoteTimerDisplay();
	GroupEventVoteWidgetInstance->SetVisibility(ESlateVisibility::Visible);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(GroupEventVoteWidgetInstance->TakeWidget());
	OwnerController->SetInputMode(InputMode);
	OwnerController->bShowMouseCursor = true;
	OwnerController->bEnableClickEvents = true;
	OwnerController->bEnableMouseOverEvents = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			GroupEventVoteCountdownHandle,
			this,
			&UGroupEventHUDComponent::TickGroupEventVoteCountdown,
			1.0f,
			true
		);
	}
}

void UGroupEventHUDComponent::FinishGroupEventVoteUI(FName EventID, const FText& ResultText)
{
	if (ActiveGroupVoteEventID != NAME_None && EventID != ActiveGroupVoteEventID)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GroupEventVoteCountdownHandle);
		World->GetTimerManager().ClearTimer(GroupEventVoteCloseHandle);
	}

	DisableGroupVoteInteractions();
	ActiveGroupVoteEventID = EventID;

	if (GroupEventVoteTitleText)
	{
		GroupEventVoteTitleText->SetText(FText::FromString(TEXT("Result")));
	}

	if (GroupEventVoteDescriptionText)
	{
		GroupEventVoteDescriptionText->SetText(ResultText);
	}

	if (GroupEventVoteTimerText)
	{
		GroupEventVoteTimerText->SetText(FText::GetEmpty());
	}

	if (GroupEventVoteWidgetInstance)
	{
		GroupEventVoteWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			GroupEventVoteCloseHandle,
			this,
			&UGroupEventHUDComponent::CloseGroupEventVoteUI,
			2.5f,
			false
		);
	}
}

void UGroupEventHUDComponent::CloseGroupEventVoteUI()
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GroupEventVoteCountdownHandle);
		World->GetTimerManager().ClearTimer(GroupEventVoteCloseHandle);
	}

	if (GroupEventVoteWidgetInstance)
	{
		GroupEventVoteWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}

	ActiveGroupVoteEventID = NAME_None;
	GroupEventVoteRemainingSeconds = 0;
	bHasSubmittedGroupVote = false;

	FInputModeGameOnly InputMode;
	OwnerController->SetInputMode(InputMode);
	OwnerController->bShowMouseCursor = false;
	OwnerController->bEnableClickEvents = false;
	OwnerController->bEnableMouseOverEvents = false;
}

void UGroupEventHUDComponent::PopulateGroupEventVoteCandidates()
{
	AMyPlayerController* OwnerController = GetOwnerController();
	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!OwnerController || !GameState)
	{
		return;
	}

	APlayerState* LocalPlayerState = OwnerController->PlayerState;
	int32 CandidateIndex = 0;
	for (APlayerState* CandidatePlayerState : GameState->PlayerArray)
	{
		if (!CandidatePlayerState || CandidatePlayerState == LocalPlayerState)
		{
			continue;
		}

		if (!A302RoomScope::ArePlayersInSameLogicalRoom(LocalPlayerState, CandidatePlayerState))
		{
			continue;
		}

		if (const AA302PlayerState* ExtendedPlayerState = Cast<AA302PlayerState>(CandidatePlayerState))
		{
			if (!ExtendedPlayerState->bIsAlive || ExtendedPlayerState->bIsEscaped)
			{
				continue;
			}
		}

		if (CandidateIndex >= VoteUserSlotWidgets.Num())
		{
			break;
		}

		if (UVoteClickableUserWidget* VoteSlotWidget = VoteUserSlotWidgets[CandidateIndex])
		{
			VoteSlotWidget->SetupCandidate(CandidatePlayerState->GetPlayerId(), ResolvePlayerDisplayName(CandidatePlayerState));
			VoteSlotWidget->SetCandidateVisible(true);
		}

		++CandidateIndex;
	}

	for (int32 SlotIndex = CandidateIndex; SlotIndex < VoteUserSlotWidgets.Num(); ++SlotIndex)
	{
		if (UVoteClickableUserWidget* VoteSlotWidget = VoteUserSlotWidgets[SlotIndex])
		{
			VoteSlotWidget->SetCandidateVisible(false);
		}
	}
}

void UGroupEventHUDComponent::UpdateGroupEventVoteTimerDisplay()
{
	if (GroupEventVoteTimerText)
	{
		GroupEventVoteTimerText->SetText(FText::AsNumber(GroupEventVoteRemainingSeconds));
	}
}

void UGroupEventHUDComponent::TickGroupEventVoteCountdown()
{
	GroupEventVoteRemainingSeconds = FMath::Max(0, GroupEventVoteRemainingSeconds - 1);
	UpdateGroupEventVoteTimerDisplay();

	if (GroupEventVoteRemainingSeconds <= 0)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(GroupEventVoteCountdownHandle);
		}
	}
}

void UGroupEventHUDComponent::DisableGroupVoteInteractions()
{
	for (UVoteClickableUserWidget* VoteSlotWidget : VoteUserSlotWidgets)
	{
		if (VoteSlotWidget)
		{
			VoteSlotWidget->SetVotingEnabled(false);
		}
	}
}

void UGroupEventHUDComponent::HandleLocalGroupVoteSelection(int32 TargetPlayerId)
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController || bHasSubmittedGroupVote || ActiveGroupVoteEventID.IsNone())
	{
		return;
	}

	bHasSubmittedGroupVote = true;
	DisableGroupVoteInteractions();

	for (UVoteClickableUserWidget* VoteSlotWidget : VoteUserSlotWidgets)
	{
		if (VoteSlotWidget)
		{
			VoteSlotWidget->SetSelected(VoteSlotWidget->GetTargetPlayerId() == TargetPlayerId);
		}
	}

	if (GroupEventVoteDescriptionText)
	{
		GroupEventVoteDescriptionText->SetText(FText::FromString(TEXT("투표가 완료되었습니다.")));
	}

	if (UPlayerEventComponent* EventComponent = OwnerController->GetPlayerEventComponent())
	{
		EventComponent->RequestSubmitGroupVote(ActiveGroupVoteEventID, TargetPlayerId);
	}
}

UTextBlock* UGroupEventHUDComponent::FindGroupEventVoteText(const FName& WidgetName) const
{
	return GroupEventVoteWidgetInstance
		? Cast<UTextBlock>(GroupEventVoteWidgetInstance->GetWidgetFromName(WidgetName))
		: nullptr;
}

UVoteClickableUserWidget* UGroupEventHUDComponent::FindVoteUserSlot(int32 SlotIndex) const
{
	if (!GroupEventVoteWidgetInstance || SlotIndex < 0 || SlotIndex >= GroupEventVoteSlotCount)
	{
		return nullptr;
	}

	const FName SlotWidgetName(*FString::Printf(TEXT("VoteUserSlot%d"), SlotIndex + 1));
	return Cast<UVoteClickableUserWidget>(GroupEventVoteWidgetInstance->GetWidgetFromName(SlotWidgetName));
}

FString UGroupEventHUDComponent::ResolvePlayerDisplayName(const APlayerState* TargetPlayerState) const
{
	if (!TargetPlayerState)
	{
		return TEXT("Unknown");
	}

	const FString PlayerName = TargetPlayerState->GetPlayerName();
	return ClampGroupEventNicknameForUi(PlayerName.IsEmpty() ? GetNameSafe(TargetPlayerState) : PlayerName);
}

