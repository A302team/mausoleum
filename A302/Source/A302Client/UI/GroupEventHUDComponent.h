#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GroupEventHUDComponent.generated.h"

class AMyPlayerController;
class APlayerState;
class UTextBlock;
class UUserWidget;
class UVoteClickableUserWidget;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302CLIENT_API UGroupEventHUDComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGroupEventHUDComponent();

	void ShowGroupEventVoteUI(TSubclassOf<UUserWidget> GroupEventVoteWidgetClass, FName EventID, const FText& EventTitle, const FText& EventDescription, float VoteDuration);
	void FinishGroupEventVoteUI(FName EventID, const FText& ResultText);
	void CloseGroupEventVoteUI();

private:
	AMyPlayerController* GetOwnerController() const;
	void InitializeGroupEventVoteWidget(TSubclassOf<UUserWidget> GroupEventVoteWidgetClass);
	void PopulateGroupEventVoteCandidates();
	void UpdateGroupEventVoteTimerDisplay();
	void TickGroupEventVoteCountdown();
	void DisableGroupVoteInteractions();
	void HandleLocalGroupVoteSelection(int32 TargetPlayerId);
	UTextBlock* FindGroupEventVoteText(const FName& WidgetName) const;
	UVoteClickableUserWidget* FindVoteUserSlot(int32 SlotIndex) const;
	FString ResolvePlayerDisplayName(const APlayerState* TargetPlayerState) const;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> GroupEventVoteWidgetInstance;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GroupEventVoteTimerText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GroupEventVoteTitleText = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GroupEventVoteDescriptionText = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UVoteClickableUserWidget>> VoteUserSlotWidgets;

	FTimerHandle GroupEventVoteCountdownHandle;
	FTimerHandle GroupEventVoteCloseHandle;

	FName ActiveGroupVoteEventID = NAME_None;
	int32 GroupEventVoteRemainingSeconds = 0;
	bool bHasSubmittedGroupVote = false;
};

