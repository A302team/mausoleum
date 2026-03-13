#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "VoteClickableUserWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnVoteClickableUserSelected, int32);

UCLASS()
class A302_API UVoteClickableUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetupCandidate(int32 InTargetPlayerId, const FString& InPlayerName);
	void SetCandidateVisible(bool bVisible);
	void SetSelected(bool bSelected);
	void SetVotingEnabled(bool bEnabled);
	int32 GetTargetPlayerId() const { return TargetPlayerId; }

	FOnVoteClickableUserSelected OnVoteClickableUserSelected;

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UBorder> VoteClickableBorder = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> VoteUserImage = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> VoteUserName = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> SelectedOverlay = nullptr;

private:
	int32 TargetPlayerId = INDEX_NONE;
	bool bVotingEnabled = true;
};
