#include "Character/Components/PlayerHUDComponent.h"

#include "Character/Components/PlayerEventComponent.h"
#include "Character/Components/MaliceComponent.h"
#include "Character/Components/QuickSlotComponent.h"
#include "Character/MyPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameData/Items/ItemDefinition.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameMode/A302PlayerState.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/PersonalEventWidget.h"
#include "UI/VoteClickableUserWidget.h"
#include "Room/RoomScopeRules.h"

namespace
{
	constexpr int32 PlayerHUDVoteSlotCount = 6;
	constexpr int32 PlayerHUDQuickSlotCount = 5;
	constexpr int32 MaxInspectMaliceTargets = 5;

	ACharacter* FindCharacterForPlayerState(const UObject* WorldContextObject, const APlayerState* TargetPlayerState)
	{
		if (!WorldContextObject || !TargetPlayerState)
		{
			return nullptr;
		}

		UWorld* World = WorldContextObject->GetWorld();
		if (!World)
		{
			return nullptr;
		}

		for (TActorIterator<ACharacter> It(World); It; ++It)
		{
			if (It->GetPlayerState() == TargetPlayerState)
			{
				return *It;
			}
		}

		return nullptr;
	}
}

UPlayerHUDComponent::UPlayerHUDComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

AMyPlayerController* UPlayerHUDComponent::GetOwnerController() const
{
	return Cast<AMyPlayerController>(GetOwner());
}

void UPlayerHUDComponent::ShowPersonalEventUI(TSubclassOf<UPersonalEventWidget> PersonalEventWidgetClass, FName EventID, const FText& EventTitle, const FText& EventDescription, const TArray<FText>& Choices)
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController || !PersonalEventWidgetClass)
	{
		return;
	}

	OwnerController->FlushPressedKeys();

	if (!PersonalEventWidgetInstance)
	{
		PersonalEventWidgetInstance = CreateWidget<UPersonalEventWidget>(OwnerController, PersonalEventWidgetClass);
	}

	if (!PersonalEventWidgetInstance)
	{
		return;
	}

	PersonalEventWidgetInstance->SetupEventUI(EventID, EventTitle, EventDescription, Choices);
	if (!PersonalEventWidgetInstance->IsInViewport())
	{
		PersonalEventWidgetInstance->AddToViewport(120);
	}

	PersonalEventWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	OwnerController->bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(PersonalEventWidgetInstance->TakeWidget());
	OwnerController->SetInputMode(InputMode);
}

void UPlayerHUDComponent::InitializeGroupEventVoteWidget(TSubclassOf<UUserWidget> GroupEventVoteWidgetClass)
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
	for (int32 SlotIndex = 0; SlotIndex < PlayerHUDVoteSlotCount; ++SlotIndex)
	{
		if (UVoteClickableUserWidget* VoteSlotWidget = FindVoteUserSlot(SlotIndex))
		{
			VoteUserSlotWidgets.Add(VoteSlotWidget);
			VoteSlotWidget->OnVoteClickableUserSelected.RemoveAll(this);
			VoteSlotWidget->OnVoteClickableUserSelected.AddUObject(this, &UPlayerHUDComponent::HandleLocalGroupVoteSelection);
		}
	}
}

void UPlayerHUDComponent::ShowGroupEventVoteUI(TSubclassOf<UUserWidget> GroupEventVoteWidgetClass, FName EventID, const FText& EventTitle, const FText& EventDescription, float VoteDuration)
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
			&UPlayerHUDComponent::TickGroupEventVoteCountdown,
			1.0f,
			true
		);
	}
}

void UPlayerHUDComponent::FinishGroupEventVoteUI(FName EventID, const FText& ResultText)
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
			&UPlayerHUDComponent::CloseGroupEventVoteUI,
			2.5f,
			false
		);
	}
}

void UPlayerHUDComponent::CloseGroupEventVoteUI()
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

void UPlayerHUDComponent::PopulateGroupEventVoteCandidates()
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

void UPlayerHUDComponent::UpdateGroupEventVoteTimerDisplay()
{
	if (GroupEventVoteTimerText)
	{
		GroupEventVoteTimerText->SetText(FText::AsNumber(GroupEventVoteRemainingSeconds));
	}
}

void UPlayerHUDComponent::TickGroupEventVoteCountdown()
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

void UPlayerHUDComponent::DisableGroupVoteInteractions()
{
	for (UVoteClickableUserWidget* VoteSlotWidget : VoteUserSlotWidgets)
	{
		if (VoteSlotWidget)
		{
			VoteSlotWidget->SetVotingEnabled(false);
		}
	}
}

void UPlayerHUDComponent::HandleLocalGroupVoteSelection(int32 TargetPlayerId)
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

UTextBlock* UPlayerHUDComponent::FindGroupEventVoteText(const FName& WidgetName) const
{
	return GroupEventVoteWidgetInstance
		? Cast<UTextBlock>(GroupEventVoteWidgetInstance->GetWidgetFromName(WidgetName))
		: nullptr;
}

UVoteClickableUserWidget* UPlayerHUDComponent::FindVoteUserSlot(int32 SlotIndex) const
{
	if (!GroupEventVoteWidgetInstance || SlotIndex < 0 || SlotIndex >= PlayerHUDVoteSlotCount)
	{
		return nullptr;
	}

	const FName SlotWidgetName(*FString::Printf(TEXT("VoteUserSlot%d"), SlotIndex + 1));
	return Cast<UVoteClickableUserWidget>(GroupEventVoteWidgetInstance->GetWidgetFromName(SlotWidgetName));
}

FString UPlayerHUDComponent::ResolvePlayerDisplayName(const APlayerState* TargetPlayerState) const
{
	if (!TargetPlayerState)
	{
		return TEXT("Unknown");
	}

	const FString PlayerName = TargetPlayerState->GetPlayerName();
	return PlayerName.IsEmpty() ? GetNameSafe(TargetPlayerState) : PlayerName;
}

void UPlayerHUDComponent::InitializeInGameHUD(TSubclassOf<UUserWidget> InQuickSlotBarClass, TSubclassOf<UUserWidget> InInGameSettingClass, TSubclassOf<UUserWidget> InInspectMaliceWidgetClass)
{
	QuickSlotBarClass = InQuickSlotBarClass;
	InGameSettingClass = InInGameSettingClass;
	InspectMaliceWidgetClass = InInspectMaliceWidgetClass;

	InitializeQuickSlotWidget();
	InitializeInGameSettingWidget();
	InitializeInspectMaliceWidget();
}

void UPlayerHUDComponent::RefreshQuickSlotBinding()
{
	BindQuickSlotComponent();
	SyncQuickSlotUIFromComponent();
}

void UPlayerHUDComponent::ToggleInGameSettingMenu()
{
	if (IsInGameSettingMenuOpen())
	{
		CloseInGameSettingMenu();
		return;
	}

	OpenInGameSettingMenu();
}

bool UPlayerHUDComponent::IsInGameSettingMenuOpen() const
{
	return InGameSettingWidget && InGameSettingWidget->GetVisibility() == ESlateVisibility::Visible;
}

void UPlayerHUDComponent::ShowPublicMaliceAnnouncement(const FString& PlayerName, int32 MaliceCount)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PublicMaliceAnnouncementHideTimerHandle);
	}

	if (UTextBlock* UserText = FindPublicMaliceAnnouncementText(TEXT("PublicMaliceBorderUser")))
	{
		UserText->SetText(FText::FromString(PlayerName));
	}

	if (UTextBlock* MaliceNumText = FindPublicMaliceAnnouncementText(TEXT("PublicMaliceNum")))
	{
		MaliceNumText->SetText(FText::AsNumber(FMath::Max(0, MaliceCount)));
	}

	SetPublicMaliceAnnouncementVisible(true);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PublicMaliceAnnouncementHideTimerHandle,
			this,
			&UPlayerHUDComponent::HidePublicMaliceAnnouncement,
			5.0f,
			false
		);
	}
}

void UPlayerHUDComponent::ShowInspectMaliceSelectionWidget()
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController || !OwnerController->IsLocalController())
	{
		return;
	}

	InitializeInspectMaliceWidget();
	if (!InspectMaliceWidgetInstance)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InspectMaliceHideTimerHandle);
	}

	ResetInspectMaliceSelectionWidget();
	PopulateInspectMaliceSelectionWidget();
	InspectMaliceWidgetInstance->SetVisibility(ESlateVisibility::Visible);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(InspectMaliceWidgetInstance->TakeWidget());
	InputMode.SetHideCursorDuringCapture(false);
	OwnerController->SetInputMode(InputMode);

	OwnerController->bShowMouseCursor = true;
	OwnerController->bEnableClickEvents = true;
	OwnerController->bEnableMouseOverEvents = true;
}

bool UPlayerHUDComponent::UpdateShieldCountText(int32 ShieldCount)
{
	UTextBlock* ShieldCountText = FindShieldCountText();
	if (!ShieldCountText)
	{
		return false;
	}

	ShieldCountText->SetText(FText::FromString(FString::Printf(TEXT("Shield : %d"), FMath::Max(0, ShieldCount))));
	ShieldCountText->SetVisibility(ESlateVisibility::Visible);
	return true;
}

bool UPlayerHUDComponent::UpdateMaliceCountText(int32 MaliceCount)
{
	UTextBlock* MaliceCountText = FindMaliceCountText();
	if (!MaliceCountText)
	{
		return false;
	}

	MaliceCountText->SetText(FText::FromString(FString::Printf(TEXT("Malice : %d"), FMath::Max(0, MaliceCount))));
	MaliceCountText->SetVisibility(ESlateVisibility::Visible);
	return true;
}

bool UPlayerHUDComponent::UpdateItemTimerText(float RemainingSeconds)
{
	UTextBlock* ItemTimerText = FindItemTimerText();
	if (!ItemTimerText)
	{
		return false;
	}

	const int32 SafeSeconds = FMath::Max(0, FMath::CeilToInt(RemainingSeconds));
	ItemTimerText->SetText(FText::FromString(FString::Printf(TEXT("%d"), SafeSeconds)));
	return true;
}

void UPlayerHUDComponent::SetItemTimerVisible(bool bVisible)
{
	if (UTextBlock* ItemTimerText = FindItemTimerText())
	{
		ItemTimerText->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void UPlayerHUDComponent::InitializeQuickSlotWidget()
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController || !QuickSlotBarClass)
	{
		return;
	}

	if (!QuickSlotBarWidget)
	{
		QuickSlotBarWidget = CreateWidget<UUserWidget>(OwnerController, QuickSlotBarClass);
		if (!QuickSlotBarWidget)
		{
			return;
		}

		QuickSlotBarWidget->AddToViewport();
	}

	InitializeQuickSlotVisualState();
	BindQuickSlotComponent();
	SyncQuickSlotUIFromComponent();
	UpdateShieldCountText(0);
	UpdateMaliceCountText(0);
	UpdateItemTimerText(30.0f);
	SetItemTimerVisible(false);
}

void UPlayerHUDComponent::InitializeQuickSlotVisualState()
{
	for (int32 SlotIndex = 0; SlotIndex < PlayerHUDQuickSlotCount; ++SlotIndex)
	{
		if (UTextBlock* ItemNameText = FindQuickSlotItemNameText(SlotIndex))
		{
			ItemNameText->SetText(FText::GetEmpty());
			ItemNameText->SetVisibility(ESlateVisibility::Hidden);
		}

		if (UUserWidget* QuickSlotWidget = FindQuickSlotWidget(SlotIndex))
		{
			if (UImage* KnifeIcon = Cast<UImage>(QuickSlotWidget->GetWidgetFromName(TEXT("KnifeIcon"))))
			{
				KnifeIcon->SetBrushFromTexture(nullptr, true);
				KnifeIcon->SetVisibility(ESlateVisibility::Hidden);
			}
			else if (UImage* ItemIcon = Cast<UImage>(QuickSlotWidget->GetWidgetFromName(TEXT("ItemIcon"))))
			{
				ItemIcon->SetBrushFromTexture(nullptr, true);
				ItemIcon->SetVisibility(ESlateVisibility::Hidden);
			}

			if (UImage* SelectedImage = Cast<UImage>(QuickSlotWidget->GetWidgetFromName(TEXT("ItemSelected"))))
			{
				SelectedImage->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}

	if (UTextBlock* UserText = FindPublicMaliceAnnouncementText(TEXT("PublicMaliceBorderUser")))
	{
		UserText->SetText(FText::GetEmpty());
	}

	if (UTextBlock* MaliceNumText = FindPublicMaliceAnnouncementText(TEXT("PublicMaliceNum")))
	{
		MaliceNumText->SetText(FText::GetEmpty());
	}

	SetPublicMaliceAnnouncementVisible(false);
}

void UPlayerHUDComponent::BindQuickSlotComponent()
{
	if (BoundQuickSlotComponent)
	{
		BoundQuickSlotComponent->OnQuickSlotSelectionChanged.RemoveAll(this);
		BoundQuickSlotComponent->OnQuickSlotItemChanged.RemoveAll(this);
		BoundQuickSlotComponent = nullptr;
	}

	AMyPlayerController* OwnerController = GetOwnerController();
	ACharacter* ControlledCharacter = Cast<ACharacter>(OwnerController ? OwnerController->GetPawn() : nullptr);
	if (!ControlledCharacter)
	{
		return;
	}

	BoundQuickSlotComponent = ControlledCharacter->FindComponentByClass<UQuickSlotComponent>();
	if (!BoundQuickSlotComponent)
	{
		return;
	}

	BoundQuickSlotComponent->OnQuickSlotSelectionChanged.AddUObject(this, &UPlayerHUDComponent::HandleQuickSlotSelectionChanged);
	BoundQuickSlotComponent->OnQuickSlotItemChanged.AddUObject(this, &UPlayerHUDComponent::HandleQuickSlotItemChanged);
}

void UPlayerHUDComponent::SyncQuickSlotUIFromComponent()
{
	if (!BoundQuickSlotComponent)
	{
		return;
	}

	for (int32 SlotIndex = 0; SlotIndex < PlayerHUDQuickSlotCount; ++SlotIndex)
	{
		HandleQuickSlotItemChanged(SlotIndex, BoundQuickSlotComponent->GetItemDefinitionAtSlot(SlotIndex));
	}

	HandleQuickSlotSelectionChanged(BoundQuickSlotComponent->GetSelectedSlotIndex());
}

void UPlayerHUDComponent::HandleQuickSlotSelectionChanged(int32 SelectedSlotIndex)
{
	UpdateQuickSlotSelectionVisual(SelectedSlotIndex);
}

void UPlayerHUDComponent::HandleQuickSlotItemChanged(int32 SlotIndex, const UItemDefinition* ItemDefinition)
{
	if (!ItemDefinition)
	{
		UpdateQuickSlotItemVisual(SlotIndex, FText::GetEmpty(), nullptr);
		return;
	}

	const FText ItemName = ItemDefinition->DisplayName.IsEmpty()
		? FText::FromName(ItemDefinition->ItemId)
		: ItemDefinition->DisplayName;

	UpdateQuickSlotItemVisual(SlotIndex, ItemName, ItemDefinition->Icon);
}

UUserWidget* UPlayerHUDComponent::FindQuickSlotWidget(int32 SlotIndex) const
{
	if (!QuickSlotBarWidget || SlotIndex < 0 || SlotIndex >= PlayerHUDQuickSlotCount)
	{
		return nullptr;
	}

	const FName SlotWidgetName(*FString::Printf(TEXT("QuickSlot%d"), SlotIndex + 1));
	return Cast<UUserWidget>(QuickSlotBarWidget->GetWidgetFromName(SlotWidgetName));
}

UTextBlock* UPlayerHUDComponent::FindQuickSlotItemNameText(int32 SlotIndex) const
{
	if (UUserWidget* QuickSlotWidget = FindQuickSlotWidget(SlotIndex))
	{
		return Cast<UTextBlock>(QuickSlotWidget->GetWidgetFromName(TEXT("ItemName")));
	}

	return nullptr;
}

UTextBlock* UPlayerHUDComponent::FindShieldCountText() const
{
	return QuickSlotBarWidget ? Cast<UTextBlock>(QuickSlotBarWidget->GetWidgetFromName(TEXT("ShieldCount"))) : nullptr;
}

UTextBlock* UPlayerHUDComponent::FindMaliceCountText() const
{
	return QuickSlotBarWidget ? Cast<UTextBlock>(QuickSlotBarWidget->GetWidgetFromName(TEXT("MaliceCount"))) : nullptr;
}

UTextBlock* UPlayerHUDComponent::FindItemTimerText() const
{
	return QuickSlotBarWidget ? Cast<UTextBlock>(QuickSlotBarWidget->GetWidgetFromName(TEXT("ItemTimer"))) : nullptr;
}

UWidget* UPlayerHUDComponent::FindPublicMaliceAnnouncementWidget() const
{
	return QuickSlotBarWidget ? QuickSlotBarWidget->GetWidgetFromName(TEXT("PublicMaliceBorder")) : nullptr;
}

UTextBlock* UPlayerHUDComponent::FindPublicMaliceAnnouncementText(const FName& WidgetName) const
{
	return QuickSlotBarWidget ? Cast<UTextBlock>(QuickSlotBarWidget->GetWidgetFromName(WidgetName)) : nullptr;
}

void UPlayerHUDComponent::SetPublicMaliceAnnouncementVisible(bool bVisible)
{
	if (UWidget* PublicMaliceWidget = FindPublicMaliceAnnouncementWidget())
	{
		PublicMaliceWidget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

void UPlayerHUDComponent::HidePublicMaliceAnnouncement()
{
	SetPublicMaliceAnnouncementVisible(false);
}

bool UPlayerHUDComponent::UpdateQuickSlotItemVisual(int32 SlotIndex, const FText& ItemName, UTexture2D* ItemIcon)
{
	UUserWidget* QuickSlotWidget = FindQuickSlotWidget(SlotIndex);
	if (!QuickSlotWidget)
	{
		return false;
	}

	bool bNameUpdated = false;
	bool bIconWidgetFound = false;
	bool bIconTextureSet = false;

	if (UTextBlock* ItemNameText = Cast<UTextBlock>(QuickSlotWidget->GetWidgetFromName(TEXT("ItemName"))))
	{
		ItemNameText->SetText(ItemName);
		ItemNameText->SetVisibility(ESlateVisibility::Visible);
		bNameUpdated = true;
	}

	UImage* ItemIconImage = Cast<UImage>(QuickSlotWidget->GetWidgetFromName(TEXT("KnifeIcon")));
	if (!ItemIconImage)
	{
		ItemIconImage = Cast<UImage>(QuickSlotWidget->GetWidgetFromName(TEXT("ItemIcon")));
	}

	if (ItemIconImage)
	{
		bIconWidgetFound = true;
		ItemIconImage->SetBrushFromTexture(ItemIcon, true);
		ItemIconImage->SetVisibility(ItemIcon ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		bIconTextureSet = (ItemIcon != nullptr);
	}

	return bNameUpdated || bIconWidgetFound || bIconTextureSet;
}

void UPlayerHUDComponent::UpdateQuickSlotSelectionVisual(int32 SelectedSlotIndex)
{
	for (int32 SlotIndex = 0; SlotIndex < PlayerHUDQuickSlotCount; ++SlotIndex)
	{
		if (UUserWidget* QuickSlotWidget = FindQuickSlotWidget(SlotIndex))
		{
			if (UImage* SelectedImage = Cast<UImage>(QuickSlotWidget->GetWidgetFromName(TEXT("ItemSelected"))))
			{
				SelectedImage->SetVisibility(SlotIndex == SelectedSlotIndex ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
			}
		}
	}
}

void UPlayerHUDComponent::InitializeInGameSettingWidget()
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController || !InGameSettingClass)
	{
		return;
	}

	if (!InGameSettingWidget)
	{
		InGameSettingWidget = CreateWidget<UUserWidget>(OwnerController, InGameSettingClass);
		if (!InGameSettingWidget)
		{
			return;
		}

		InGameSettingWidget->AddToViewport(100);
		InGameSettingWidget->SetVisibility(ESlateVisibility::Hidden);
	}

	ResolutionComboBox = Cast<UComboBoxString>(InGameSettingWidget->GetWidgetFromName(TEXT("ResolutionComboBox")));
	ResolutionApplyBtn = Cast<UButton>(InGameSettingWidget->GetWidgetFromName(TEXT("ResolutionApplyBtn")));
	ExitBtn = Cast<UButton>(InGameSettingWidget->GetWidgetFromName(TEXT("ExitBtn")));

	if (ResolutionApplyBtn)
	{
		ResolutionApplyBtn->OnClicked.RemoveDynamic(this, &UPlayerHUDComponent::OnResolutionApplyClicked);
		ResolutionApplyBtn->OnClicked.AddDynamic(this, &UPlayerHUDComponent::OnResolutionApplyClicked);
	}

	if (ExitBtn)
	{
		ExitBtn->OnClicked.RemoveDynamic(this, &UPlayerHUDComponent::OnExitClicked);
		ExitBtn->OnClicked.AddDynamic(this, &UPlayerHUDComponent::OnExitClicked);
	}

	SyncResolutionComboToCurrent();
}

void UPlayerHUDComponent::OpenInGameSettingMenu()
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController)
	{
		return;
	}

	if (!InGameSettingWidget)
	{
		InitializeInGameSettingWidget();
	}

	if (!InGameSettingWidget)
	{
		return;
	}

	SyncResolutionComboToCurrent();
	InGameSettingWidget->SetVisibility(ESlateVisibility::Visible);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(InGameSettingWidget->TakeWidget());
	InputMode.SetHideCursorDuringCapture(false);
	OwnerController->SetInputMode(InputMode);
	OwnerController->bShowMouseCursor = true;
	OwnerController->bEnableClickEvents = true;
	OwnerController->bEnableMouseOverEvents = true;
	OwnerController->SetIgnoreLookInput(true);
	OwnerController->SetIgnoreMoveInput(true);
}

void UPlayerHUDComponent::CloseInGameSettingMenu()
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController)
	{
		return;
	}

	if (InGameSettingWidget)
	{
		InGameSettingWidget->SetVisibility(ESlateVisibility::Hidden);
	}

	FInputModeGameOnly InputMode;
	OwnerController->SetInputMode(InputMode);
	OwnerController->bShowMouseCursor = false;
	OwnerController->bEnableClickEvents = false;
	OwnerController->bEnableMouseOverEvents = false;
	OwnerController->SetIgnoreLookInput(false);
	OwnerController->SetIgnoreMoveInput(false);
}

void UPlayerHUDComponent::SyncResolutionComboToCurrent()
{
	if (!ResolutionComboBox)
	{
		return;
	}

	FIntPoint CurrentResolution = FIntPoint::ZeroValue;
	if (UGameUserSettings* GameUserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		CurrentResolution = GameUserSettings->GetScreenResolution();
	}

	if (CurrentResolution.X <= 0 || CurrentResolution.Y <= 0)
	{
		if (AMyPlayerController* OwnerController = GetOwnerController())
		{
			int32 ViewportX = 0;
			int32 ViewportY = 0;
			OwnerController->GetViewportSize(ViewportX, ViewportY);
			CurrentResolution = FIntPoint(ViewportX, ViewportY);
		}
	}

	if (CurrentResolution.X <= 0 || CurrentResolution.Y <= 0)
	{
		return;
	}

	const int32 OptionCount = ResolutionComboBox->GetOptionCount();
	for (int32 OptionIndex = 0; OptionIndex < OptionCount; ++OptionIndex)
	{
		const FString OptionText = ResolutionComboBox->GetOptionAtIndex(OptionIndex);
		FIntPoint ParsedResolution = FIntPoint::ZeroValue;
		if (TryParseResolutionString(OptionText, ParsedResolution) && ParsedResolution == CurrentResolution)
		{
			ResolutionComboBox->SetSelectedOption(OptionText);
			return;
		}
	}
}

bool UPlayerHUDComponent::TryParseResolutionString(const FString& InOption, FIntPoint& OutResolution) const
{
	FString Option = InOption;
	Option.TrimStartAndEndInline();

	FString XString;
	FString YString;
	if (!Option.Split(TEXT("x"), &XString, &YString, ESearchCase::IgnoreCase, ESearchDir::FromStart))
	{
		return false;
	}

	const int32 X = FCString::Atoi(*XString);
	const int32 Y = FCString::Atoi(*YString);
	if (X <= 0 || Y <= 0)
	{
		return false;
	}

	OutResolution = FIntPoint(X, Y);
	return true;
}

void UPlayerHUDComponent::OnResolutionApplyClicked()
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController || !ResolutionComboBox)
	{
		return;
	}

	const FString SelectedOption = ResolutionComboBox->GetSelectedOption();
	if (SelectedOption.IsEmpty())
	{
		return;
	}

	FIntPoint TargetResolution = FIntPoint::ZeroValue;
	if (!TryParseResolutionString(SelectedOption, TargetResolution))
	{
		return;
	}

	if (UGameUserSettings* GameUserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		GameUserSettings->SetFullscreenMode(EWindowMode::Windowed);
		GameUserSettings->SetScreenResolution(TargetResolution);
		GameUserSettings->ApplyResolutionSettings(false);
		GameUserSettings->ApplySettings(false);
		GameUserSettings->SaveSettings();

		const FString SetResCommand = FString::Printf(TEXT("r.SetRes %dx%dw"), TargetResolution.X, TargetResolution.Y);
		OwnerController->ConsoleCommand(SetResCommand, true);
		SyncResolutionComboToCurrent();
	}
}

void UPlayerHUDComponent::OnExitClicked()
{
	if (AMyPlayerController* OwnerController = GetOwnerController())
	{
		UKismetSystemLibrary::QuitGame(this, OwnerController, EQuitPreference::Quit, false);
	}
}

void UPlayerHUDComponent::InitializeInspectMaliceWidget()
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController || !InspectMaliceWidgetClass)
	{
		return;
	}

	if (!InspectMaliceWidgetInstance)
	{
		InspectMaliceWidgetInstance = CreateWidget<UUserWidget>(OwnerController, InspectMaliceWidgetClass);
		if (!InspectMaliceWidgetInstance)
		{
			return;
		}

		InspectMaliceWidgetInstance->AddToViewport(121);
	}

	InspectMaliceWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);

	if (UButton* Player1Button = FindInspectMaliceButton(TEXT("UserBtn1")))
	{
		Player1Button->OnClicked.RemoveDynamic(this, &UPlayerHUDComponent::OnInspectMalicePlayer1Clicked);
		Player1Button->OnClicked.AddDynamic(this, &UPlayerHUDComponent::OnInspectMalicePlayer1Clicked);
	}

	if (UButton* Player2Button = FindInspectMaliceButton(TEXT("UserBtn2")))
	{
		Player2Button->OnClicked.RemoveDynamic(this, &UPlayerHUDComponent::OnInspectMalicePlayer2Clicked);
		Player2Button->OnClicked.AddDynamic(this, &UPlayerHUDComponent::OnInspectMalicePlayer2Clicked);
	}

	if (UButton* Player3Button = FindInspectMaliceButton(TEXT("UserBtn3")))
	{
		Player3Button->OnClicked.RemoveDynamic(this, &UPlayerHUDComponent::OnInspectMalicePlayer3Clicked);
		Player3Button->OnClicked.AddDynamic(this, &UPlayerHUDComponent::OnInspectMalicePlayer3Clicked);
	}

	if (UButton* Player4Button = FindInspectMaliceButton(TEXT("UserBtn4")))
	{
		Player4Button->OnClicked.RemoveDynamic(this, &UPlayerHUDComponent::OnInspectMalicePlayer4Clicked);
		Player4Button->OnClicked.AddDynamic(this, &UPlayerHUDComponent::OnInspectMalicePlayer4Clicked);
	}

	if (UButton* Player5Button = FindInspectMaliceButton(TEXT("UserBtn5")))
	{
		Player5Button->OnClicked.RemoveDynamic(this, &UPlayerHUDComponent::OnInspectMalicePlayer5Clicked);
		Player5Button->OnClicked.AddDynamic(this, &UPlayerHUDComponent::OnInspectMalicePlayer5Clicked);
	}

	for (int32 Index = 1; Index <= PlayerHUDVoteSlotCount; ++Index)
	{
		const FName ButtonName(*FString::Printf(TEXT("UserBtn%d"), Index));
		if (UButton* HiddenButton = FindInspectMaliceButton(ButtonName))
		{
			HiddenButton->SetVisibility(ESlateVisibility::Collapsed);
			HiddenButton->SetIsEnabled(false);
		}

		const FName TextName(*FString::Printf(TEXT("UserText%d"), Index));
		if (UTextBlock* HiddenText = FindInspectMaliceText(TextName))
		{
			HiddenText->SetVisibility(ESlateVisibility::Collapsed);
			HiddenText->SetText(FText::GetEmpty());
		}
	}

	InspectMaliceSelectablePlayers.Reset();
	ResetInspectMaliceSelectionWidget();
}

void UPlayerHUDComponent::PopulateInspectMaliceSelectionWidget()
{
	InspectMaliceSelectablePlayers.Reset();

	AMyPlayerController* OwnerController = GetOwnerController();
	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!OwnerController || !GameState)
	{
		return;
	}

	APlayerState* LocalPlayerState = OwnerController->PlayerState;
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

		if (InspectMaliceSelectablePlayers.Num() >= MaxInspectMaliceTargets)
		{
			break;
		}

		InspectMaliceSelectablePlayers.Add(CandidatePlayerState);

		const int32 WidgetIndex = InspectMaliceSelectablePlayers.Num();
		const FName ButtonName(*FString::Printf(TEXT("UserBtn%d"), WidgetIndex));
		if (UButton* TargetButton = FindInspectMaliceButton(ButtonName))
		{
			TargetButton->SetVisibility(ESlateVisibility::Visible);
			TargetButton->SetIsEnabled(true);
		}

		const FName TextName(*FString::Printf(TEXT("UserText%d"), WidgetIndex));
		if (UTextBlock* TargetText = FindInspectMaliceText(TextName))
		{
			TargetText->SetText(FText::FromString(ResolvePlayerDisplayName(CandidatePlayerState)));
			TargetText->SetVisibility(ESlateVisibility::Visible);
		}
	}

	for (int32 WidgetIndex = InspectMaliceSelectablePlayers.Num() + 1; WidgetIndex <= PlayerHUDVoteSlotCount; ++WidgetIndex)
	{
		const FName ButtonName(*FString::Printf(TEXT("UserBtn%d"), WidgetIndex));
		if (UButton* HiddenButton = FindInspectMaliceButton(ButtonName))
		{
			HiddenButton->SetVisibility(ESlateVisibility::Collapsed);
			HiddenButton->SetIsEnabled(false);
		}

		const FName TextName(*FString::Printf(TEXT("UserText%d"), WidgetIndex));
		if (UTextBlock* HiddenText = FindInspectMaliceText(TextName))
		{
			HiddenText->SetVisibility(ESlateVisibility::Collapsed);
			HiddenText->SetText(FText::GetEmpty());
		}
	}
}

void UPlayerHUDComponent::ResetInspectMaliceSelectionWidget()
{
	if (UTextBlock* UserText = FindInspectMaliceText(TEXT("InspectMaliceUserText")))
	{
		UserText->SetText(FText::GetEmpty());
	}

	if (UTextBlock* MaliceNumText = FindInspectMaliceText(TEXT("InspectMaliceUserMaliceNum")))
	{
		MaliceNumText->SetText(FText::GetEmpty());
	}

	SetInspectMaliceResultVisible(false);
}

void UPlayerHUDComponent::SetInspectMaliceResultVisible(bool bVisible)
{
	UTextBlock* UserText = FindInspectMaliceText(TEXT("InspectMaliceUserText"));
	if (!UserText)
	{
		return;
	}

	if (UPanelWidget* ResultRow = UserText->GetParent())
	{
		ResultRow->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		return;
	}

	UserText->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (UTextBlock* MaliceNumText = FindInspectMaliceText(TEXT("InspectMaliceUserMaliceNum")))
	{
		MaliceNumText->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UPlayerHUDComponent::HideInspectMaliceSelectionWidget()
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController)
	{
		return;
	}

	if (InspectMaliceWidgetInstance)
	{
		InspectMaliceWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}

	FInputModeGameOnly InputMode;
	OwnerController->SetInputMode(InputMode);
	OwnerController->bShowMouseCursor = false;
	OwnerController->bEnableClickEvents = false;
	OwnerController->bEnableMouseOverEvents = false;
}

void UPlayerHUDComponent::ApplyInspectMaliceSelection(int32 EntryIndex)
{
	if (!InspectMaliceSelectablePlayers.IsValidIndex(EntryIndex))
	{
		return;
	}

	APlayerState* TargetPlayerState = InspectMaliceSelectablePlayers[EntryIndex];
	if (!TargetPlayerState)
	{
		return;
	}

	if (UTextBlock* UserText = FindInspectMaliceText(TEXT("InspectMaliceUserText")))
	{
		UserText->SetText(FText::FromString(ResolvePlayerDisplayName(TargetPlayerState)));
	}

	if (UTextBlock* MaliceNumText = FindInspectMaliceText(TEXT("InspectMaliceUserMaliceNum")))
	{
		MaliceNumText->SetText(FText::AsNumber(QueryPlayerMaliceCount(TargetPlayerState)));
	}

	SetInspectMaliceResultVisible(true);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			InspectMaliceHideTimerHandle,
			this,
			&UPlayerHUDComponent::HideInspectMaliceSelectionWidget,
			3.0f,
			false
		);
	}
}

int32 UPlayerHUDComponent::QueryPlayerMaliceCount(const APlayerState* TargetPlayerState) const
{
	const ACharacter* TargetCharacter = FindCharacterForPlayerState(this, TargetPlayerState);
	if (!TargetCharacter)
	{
		return 0;
	}

	const UMaliceComponent* MaliceComponent = TargetCharacter->FindComponentByClass<UMaliceComponent>();
	return MaliceComponent ? MaliceComponent->GetMaliceCount() : 0;
}

UButton* UPlayerHUDComponent::FindInspectMaliceButton(const FName& WidgetName) const
{
	return InspectMaliceWidgetInstance ? Cast<UButton>(InspectMaliceWidgetInstance->GetWidgetFromName(WidgetName)) : nullptr;
}

UTextBlock* UPlayerHUDComponent::FindInspectMaliceText(const FName& WidgetName) const
{
	return InspectMaliceWidgetInstance ? Cast<UTextBlock>(InspectMaliceWidgetInstance->GetWidgetFromName(WidgetName)) : nullptr;
}

void UPlayerHUDComponent::OnInspectMalicePlayer1Clicked()
{
	ApplyInspectMaliceSelection(0);
}

void UPlayerHUDComponent::OnInspectMalicePlayer2Clicked()
{
	ApplyInspectMaliceSelection(1);
}

void UPlayerHUDComponent::OnInspectMalicePlayer3Clicked()
{
	ApplyInspectMaliceSelection(2);
}

void UPlayerHUDComponent::OnInspectMalicePlayer4Clicked()
{
	ApplyInspectMaliceSelection(3);
}

void UPlayerHUDComponent::OnInspectMalicePlayer5Clicked()
{
	ApplyInspectMaliceSelection(4);
}

