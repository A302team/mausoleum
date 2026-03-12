#include "UI/VoteClickableUserWidget.h"

#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Input/Reply.h"

void UVoteClickableUserWidget::SetupCandidate(int32 InTargetPlayerId, const FString& InPlayerName)
{
	TargetPlayerId = InTargetPlayerId;

	if (VoteUserName)
	{
		VoteUserName->SetText(FText::FromString(InPlayerName));
	}

	SetSelected(false);
	SetVotingEnabled(true);
}

void UVoteClickableUserWidget::SetCandidateVisible(bool bVisible)
{
	if (!bVisible)
	{
		TargetPlayerId = INDEX_NONE;
		SetSelected(false);
		SetVotingEnabled(false);
	}

	SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UVoteClickableUserWidget::SetSelected(bool bSelected)
{
	if (SelectedOverlay)
	{
		SelectedOverlay->SetVisibility(bSelected ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void UVoteClickableUserWidget::SetVotingEnabled(bool bEnabled)
{
	bVotingEnabled = bEnabled;
	SetIsEnabled(bEnabled);
}

FReply UVoteClickableUserWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bVotingEnabled || TargetPlayerId == INDEX_NONE || InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	OnVoteClickableUserSelected.Broadcast(TargetPlayerId);
	return FReply::Handled();
}
