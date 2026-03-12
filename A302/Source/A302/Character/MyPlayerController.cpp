#include "Character/MyPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Character/Components/ItemManagerComponent.h"
#include "Character/Components/MaliceComponent.h"
#include "Character/MyCharacter.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "EnhancedInputSubsystems.h"
#include "GamePlay/Events/BaseEvent.h"
#include "GamePlay/Events/GroupEvents/BaseGroupEvent.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameMode/A302PlayerState.h"
#include "InputMappingContext.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameMode/A302GameMode.h"
#include "UI/ChatWidget.h"
#include "UI/PersonalEventWidget.h"
#include "UI/VoteClickableUserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	constexpr int32 PlayerControllerQuickSlotCount = 5;
	constexpr int32 InspectMaliceSlotCount = 6;
	constexpr int32 InspectMaliceMaxOtherPlayers = 5;
	constexpr int32 GroupEventVoteSlotCount = 6;
}

// bool AMyPlayerController::ServerRequestGameStart_Validate()
// {
//     return true;
// }

// void AMyPlayerController::ServerRequestGameStart_Implementation()
// {
//     UWorld* World = GetWorld();
//     if (!World) return;
    
//     UE_LOG(LogTemp, Log, TEXT("[PC] ServerRequestGameStart - NetMode: %d"), (int32)World->GetNetMode());
//     World->ServerTravel(TEXT("/Game/PersonalWorkSpace/sikk806/MyTestLevel?listen"));
// }

UUserWidget *AMyPlayerController::FindQuickSlotWidget(int32 SlotIndex) const
{
	if (!QuickSlotBarWidget || SlotIndex < 0 || SlotIndex >= PlayerControllerQuickSlotCount)
	{
		return nullptr;
	}

	const FName SlotWidgetName(*FString::Printf(TEXT("QuickSlot%d"), SlotIndex + 1));
	return Cast<UUserWidget>(QuickSlotBarWidget->GetWidgetFromName(SlotWidgetName));
}

UTextBlock *AMyPlayerController::FindQuickSlotItemNameText(int32 SlotIndex) const
{
	if (UUserWidget *QuickSlotWidget = FindQuickSlotWidget(SlotIndex))
	{
		return Cast<UTextBlock>(QuickSlotWidget->GetWidgetFromName(TEXT("ItemName")));
	}

	return nullptr;
}

UImage *AMyPlayerController::FindQuickSlotItemIconImage(int32 SlotIndex) const
{
	if (UUserWidget *QuickSlotWidget = FindQuickSlotWidget(SlotIndex))
	{
		if (UImage *KnifeIcon = Cast<UImage>(QuickSlotWidget->GetWidgetFromName(TEXT("KnifeIcon"))))
		{
			return KnifeIcon;
		}

		return Cast<UImage>(QuickSlotWidget->GetWidgetFromName(TEXT("ItemIcon")));
	}

	return nullptr;
}

UImage *AMyPlayerController::FindQuickSlotItemSelectedImage(int32 SlotIndex) const
{
	if (UUserWidget *QuickSlotWidget = FindQuickSlotWidget(SlotIndex))
	{
		return Cast<UImage>(QuickSlotWidget->GetWidgetFromName(TEXT("ItemSelected")));
	}

	return nullptr;
}

UButton *AMyPlayerController::FindInspectMaliceButton(const FName& WidgetName) const
{
	if (!InspectMaliceWidgetInstance)
	{
		return nullptr;
	}

	return Cast<UButton>(InspectMaliceWidgetInstance->GetWidgetFromName(WidgetName));
}

UTextBlock *AMyPlayerController::FindInspectMaliceText(const FName& WidgetName) const
{
	if (!InspectMaliceWidgetInstance)
	{
		return nullptr;
	}

	return Cast<UTextBlock>(InspectMaliceWidgetInstance->GetWidgetFromName(WidgetName));
}

UTextBlock *AMyPlayerController::FindGroupEventVoteText(const FName& WidgetName) const
{
	if (!GroupEventVoteWidgetInstance)
	{
		return nullptr;
	}

	return Cast<UTextBlock>(GroupEventVoteWidgetInstance->GetWidgetFromName(WidgetName));
}

UVoteClickableUserWidget *AMyPlayerController::FindVoteUserSlot(int32 SlotIndex) const
{
	if (!GroupEventVoteWidgetInstance || SlotIndex < 0 || SlotIndex >= GroupEventVoteSlotCount)
	{
		return nullptr;
	}

	const FName SlotWidgetName(*FString::Printf(TEXT("VoteUserSlot%d"), SlotIndex + 1));
	return Cast<UVoteClickableUserWidget>(GroupEventVoteWidgetInstance->GetWidgetFromName(SlotWidgetName));
}

UWidget *AMyPlayerController::FindPublicMaliceAnnouncementWidget() const
{
	if (!QuickSlotBarWidget)
	{
		return nullptr;
	}

	return QuickSlotBarWidget->GetWidgetFromName(TEXT("PublicMaliceBorder"));
}

UTextBlock *AMyPlayerController::FindPublicMaliceAnnouncementText(const FName& WidgetName) const
{
	if (!QuickSlotBarWidget)
	{
		return nullptr;
	}

	return Cast<UTextBlock>(QuickSlotBarWidget->GetWidgetFromName(WidgetName));
}

UTextBlock *AMyPlayerController::FindShieldCountText() const
{
	if (!QuickSlotBarWidget)
	{
		return nullptr;
	}

	return Cast<UTextBlock>(QuickSlotBarWidget->GetWidgetFromName(TEXT("ShieldCount")));
}

UTextBlock *AMyPlayerController::FindMaliceCountText() const
{
	if (!QuickSlotBarWidget)
	{
		return nullptr;
	}

	return Cast<UTextBlock>(QuickSlotBarWidget->GetWidgetFromName(TEXT("MaliceCount")));
}

UTextBlock *AMyPlayerController::FindItemTimerText() const
{
	if (!QuickSlotBarWidget)
	{
		return nullptr;
	}

	return Cast<UTextBlock>(QuickSlotBarWidget->GetWidgetFromName(TEXT("ItemTimer")));
}

bool AMyPlayerController::UpdateQuickSlotItemVisual(int32 SlotIndex, const FText &ItemName, UTexture2D *ItemIcon)
{
	bool bNameUpdated = false;
	bool bIconWidgetFound = false;
	bool bIconTextureSet = false;

	if (UTextBlock *ItemNameText = FindQuickSlotItemNameText(SlotIndex))
	{
		ItemNameText->SetText(ItemName);
		ItemNameText->SetVisibility(ESlateVisibility::Visible);
		bNameUpdated = true;
	}

	if (UImage *ItemIconImage = FindQuickSlotItemIconImage(SlotIndex))
	{
		bIconWidgetFound = true;
		ItemIconImage->SetBrushFromTexture(ItemIcon, true);

		if (ItemIcon)
		{
			ItemIconImage->SetVisibility(ESlateVisibility::Visible);
			bIconTextureSet = true;
		}
		else
		{
			ItemIconImage->SetVisibility(ESlateVisibility::Hidden);
			UE_LOG(LogTemp, Warning, TEXT("[PC] QuickSlot %d icon texture is null. Check ItemDefinition.Icon."), SlotIndex + 1);
		}
	}

	if (!bNameUpdated)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] QuickSlot %d ItemName widget not found. Expected name: ItemName"), SlotIndex + 1);
	}

	if (!bIconWidgetFound)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] QuickSlot %d Icon widget not found. Expected name: KnifeIcon (or ItemIcon)"), SlotIndex + 1);
	}

	return bNameUpdated || bIconWidgetFound || bIconTextureSet;
}

bool AMyPlayerController::UpdateQuickSlotItemName(int32 SlotIndex, const FText &ItemName)
{
	return UpdateQuickSlotItemVisual(SlotIndex, ItemName, nullptr);
}

void AMyPlayerController::UpdateQuickSlotSelectionVisual(int32 SelectedSlotIndex)
{
	for (int32 SlotIndex = 0; SlotIndex < PlayerControllerQuickSlotCount; ++SlotIndex)
	{
		if (UImage *SelectedImage = FindQuickSlotItemSelectedImage(SlotIndex))
		{
			SelectedImage->SetVisibility(
				SlotIndex == SelectedSlotIndex ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		}
	}
}

bool AMyPlayerController::UpdateShieldCountText(int32 ShieldCount)
{
	UTextBlock *ShieldCountText = FindShieldCountText();
	if (!ShieldCountText)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] ShieldCount text widget not found. Expected name: ShieldCount"));
		return false;
	}

	ShieldCountText->SetText(FText::FromString(FString::Printf(TEXT("Shield : %d"), FMath::Max(0, ShieldCount))));
	ShieldCountText->SetVisibility(ESlateVisibility::Visible);
	return true;
}

bool AMyPlayerController::UpdateMaliceCountText(int32 MaliceCount)
{
	UTextBlock *MaliceCountText = FindMaliceCountText();
	if (!MaliceCountText)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] MaliceCount text widget not found. Expected name: MaliceCount"));
		return false;
	}

	MaliceCountText->SetText(FText::FromString(FString::Printf(TEXT("Malice : %d"), FMath::Max(0, MaliceCount))));
	MaliceCountText->SetVisibility(ESlateVisibility::Visible);
	return true;
}

bool AMyPlayerController::UpdateItemTimerText(float RemainingSeconds)
{
	UTextBlock *ItemTimerText = FindItemTimerText();
	if (!ItemTimerText)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] ItemTimer text widget not found. Expected name: ItemTimer"));
		return false;
	}

	const int32 SafeSeconds = FMath::Max(0, FMath::CeilToInt(RemainingSeconds));
	ItemTimerText->SetText(FText::FromString(FString::Printf(TEXT("%d"), SafeSeconds)));
	return true;
}

void AMyPlayerController::SetItemTimerVisible(bool bVisible)
{
	if (UTextBlock *ItemTimerText = FindItemTimerText())
	{
		ItemTimerText->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

bool AMyPlayerController::IsInGameSettingMenuOpen() const
{
	return InGameSettingWidget && InGameSettingWidget->GetVisibility() == ESlateVisibility::Visible;
}

void AMyPlayerController::ShowPublicMaliceAnnouncement(const FString& PlayerName, int32 MaliceCount)
{
	if (UWorld *World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PublicMaliceAnnouncementHideTimerHandle);
	}

	if (UTextBlock *UserText = FindPublicMaliceAnnouncementText(TEXT("PublicMaliceBorderUser")))
	{
		UserText->SetText(FText::FromString(PlayerName));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] Public malice user text widget not found. Expected name: PublicMaliceBorderUser"));
	}

	if (UTextBlock *MaliceNumText = FindPublicMaliceAnnouncementText(TEXT("PublicMaliceNum")))
	{
		MaliceNumText->SetText(FText::AsNumber(FMath::Max(0, MaliceCount)));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] Public malice number text widget not found. Expected name: PublicMaliceNum"));
	}

	SetPublicMaliceAnnouncementVisible(true);

	if (UWorld *World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			PublicMaliceAnnouncementHideTimerHandle,
			this,
			&AMyPlayerController::HidePublicMaliceAnnouncement,
			5.0f,
			false
		);
	}
}

void AMyPlayerController::ToggleInGameSettingMenu()
{
	if (IsInGameSettingMenuOpen())
	{
		CloseInGameSettingMenu();
		return;
	}

	OpenInGameSettingMenu();
}

void AMyPlayerController::InitializeInGameSettingWidget()
{
	if (!InGameSettingClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] InGameSettingClass is NULL"));
		return;
	}

	if (!InGameSettingWidget)
	{
		InGameSettingWidget = CreateWidget<UUserWidget>(this, InGameSettingClass);
		if (!InGameSettingWidget)
		{
			UE_LOG(LogTemp, Error, TEXT("[PC] Failed to create InGameSettingWidget"));
			return;
		}

		InGameSettingWidget->AddToViewport(100);
		InGameSettingWidget->SetVisibility(ESlateVisibility::Hidden);
	}

	ResolutionComboBox = Cast<UComboBoxString>(InGameSettingWidget->GetWidgetFromName(TEXT("ResolutionComboBox")));
	ResolutionApplyBtn = Cast<UButton>(InGameSettingWidget->GetWidgetFromName(TEXT("ResolutionApplyBtn")));
	ExitBtn = Cast<UButton>(InGameSettingWidget->GetWidgetFromName(TEXT("ExitBtn")));

	if (!ResolutionComboBox)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] ResolutionComboBox widget not found"));
	}

	if (ResolutionApplyBtn)
	{
		ResolutionApplyBtn->OnClicked.RemoveDynamic(this, &AMyPlayerController::OnResolutionApplyClicked);
		ResolutionApplyBtn->OnClicked.AddDynamic(this, &AMyPlayerController::OnResolutionApplyClicked);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] ResolutionApplyBtn widget not found"));
	}

	if (ExitBtn)
	{
		ExitBtn->OnClicked.RemoveDynamic(this, &AMyPlayerController::OnExitClicked);
		ExitBtn->OnClicked.AddDynamic(this, &AMyPlayerController::OnExitClicked);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] ExitBtn widget not found"));
	}

	SyncResolutionComboToCurrent();
}

void AMyPlayerController::OpenInGameSettingMenu()
{
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
	SetInputMode(InputMode);

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	SetIgnoreLookInput(true);
	SetIgnoreMoveInput(true);
}

void AMyPlayerController::CloseInGameSettingMenu()
{
	if (InGameSettingWidget)
	{
		InGameSettingWidget->SetVisibility(ESlateVisibility::Hidden);
	}

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	SetIgnoreLookInput(false);
	SetIgnoreMoveInput(false);
}

void AMyPlayerController::OnExitClicked()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

AMyPlayerController::AMyPlayerController()
{
	static ConstructorHelpers::FClassFinder<UUserWidget> InspectMaliceWidgetBPClass(TEXT("/Game/WorkSpace/UI/WBP_SelectUser"));
	if (InspectMaliceWidgetBPClass.Succeeded())
	{
		InspectMaliceWidgetClass = InspectMaliceWidgetBPClass.Class;
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> GroupEventVoteWidgetBPClass(TEXT("/Game/WorkSpace/UI/GroupEvent/WBP_GroupEventVote"));
	if (GroupEventVoteWidgetBPClass.Succeeded())
	{
		GroupEventVoteWidgetClass = GroupEventVoteWidgetBPClass.Class;
	}
}

void AMyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 로컬 컨트롤러에서만 실행
	if (!IsLocalController())
		return;

	if (ULocalPlayer *LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem *Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (DefaultMappingContext)
			{
				Subsys->AddMappingContext(DefaultMappingContext, MappingPriority);
			}
		}
	}

	// 인게임 레벨일 때만 ChatWidget 생성
	FString MapName = GetWorld()->GetMapName();
	if (MapName.Contains(TEXT("MyTestLevel")))
	{
		AA302GameMode *GameMode = Cast<AA302GameMode>(UGameplayStatics::GetGameMode(this));
		if (GameMode && GameMode->ChatWidgetClass)
		{
			UChatWidget *ChatWidget = CreateWidget<UChatWidget>(this, GameMode->ChatWidgetClass);
			if (ChatWidget)
			{
				ChatWidget->AddToViewport();
				GameMode->ChatWidget = ChatWidget;
				UE_LOG(LogTemp, Log, TEXT("[PC] ChatWidget 생성 성공"));
			}
		}
	}

	if (QuickSlotBarClass)
	{
		QuickSlotBarWidget = CreateWidget<UUserWidget>(this, QuickSlotBarClass);
		if (QuickSlotBarWidget)
		{
			QuickSlotBarWidget->AddToViewport();
			InitializeQuickSlotVisualState();
			UpdateShieldCountText(0);
			UpdateMaliceCountText(0);
			UpdateItemTimerText(30.0f);
			SetItemTimerVisible(false);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[PC] QuickSlotBarClass is NULL"));
	}

	InitializeInGameSettingWidget();
	InitializeInspectMaliceWidget();
	InitializeGroupEventVoteWidget();
}

void AMyPlayerController::InitializeQuickSlotVisualState()
{
	for (int32 SlotIndex = 0; SlotIndex < PlayerControllerQuickSlotCount; ++SlotIndex)
	{
		if (UTextBlock *ItemNameText = FindQuickSlotItemNameText(SlotIndex))
		{
			ItemNameText->SetText(FText::GetEmpty());
			ItemNameText->SetVisibility(ESlateVisibility::Hidden);
		}

		if (UImage *ItemIconImage = FindQuickSlotItemIconImage(SlotIndex))
		{
			ItemIconImage->SetBrushFromTexture(nullptr, true);
			ItemIconImage->SetVisibility(ESlateVisibility::Hidden);
		}

		if (UImage *SelectedImage = FindQuickSlotItemSelectedImage(SlotIndex))
		{
			SelectedImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (UTextBlock *UserText = FindPublicMaliceAnnouncementText(TEXT("PublicMaliceBorderUser")))
	{
		UserText->SetText(FText::GetEmpty());
	}

	if (UTextBlock *MaliceNumText = FindPublicMaliceAnnouncementText(TEXT("PublicMaliceNum")))
	{
		MaliceNumText->SetText(FText::GetEmpty());
	}

	SetPublicMaliceAnnouncementVisible(false);
}

void AMyPlayerController::InitializeInspectMaliceWidget()
{
	if (!InspectMaliceWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InspectMalice] InspectMaliceWidgetClass is null."));
		return;
	}

	if (!InspectMaliceWidgetInstance)
	{
		InspectMaliceWidgetInstance = CreateWidget<UUserWidget>(this, InspectMaliceWidgetClass);
		if (!InspectMaliceWidgetInstance)
		{
			UE_LOG(LogTemp, Error, TEXT("[InspectMalice] Failed to create inspect malice widget."));
			return;
		}

		InspectMaliceWidgetInstance->AddToViewport(121);
	}

	InspectMaliceWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);

	InspectMaliceCandidatePlayerIds.Init(INDEX_NONE, InspectMaliceSlotCount);

	for (int32 Index = 0; Index < InspectMaliceSlotCount; ++Index)
	{
		const FName ButtonName(*FString::Printf(TEXT("UserBtn%d"), Index + 1));
		if (UButton* CandidateButton = FindInspectMaliceButton(ButtonName))
		{
			CandidateButton->OnClicked.Clear();

			switch (Index)
			{
			case 0:
				CandidateButton->OnClicked.AddDynamic(this, &AMyPlayerController::OnInspectMaliceUser1Clicked);
				break;
			case 1:
				CandidateButton->OnClicked.AddDynamic(this, &AMyPlayerController::OnInspectMaliceUser2Clicked);
				break;
			case 2:
				CandidateButton->OnClicked.AddDynamic(this, &AMyPlayerController::OnInspectMaliceUser3Clicked);
				break;
			case 3:
				CandidateButton->OnClicked.AddDynamic(this, &AMyPlayerController::OnInspectMaliceUser4Clicked);
				break;
			case 4:
				CandidateButton->OnClicked.AddDynamic(this, &AMyPlayerController::OnInspectMaliceUser5Clicked);
				break;
			case 5:
				CandidateButton->OnClicked.AddDynamic(this, &AMyPlayerController::OnInspectMaliceUser6Clicked);
				break;
			default:
				break;
			}

			CandidateButton->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[InspectMalice] %s widget not found."), *ButtonName.ToString());
		}

		const FName TextName(*FString::Printf(TEXT("UserText%d"), Index + 1));
		if (UTextBlock* CandidateText = FindInspectMaliceText(TextName))
		{
			CandidateText->SetText(FText::GetEmpty());
			CandidateText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	ResetInspectMaliceSelectionWidget();
}

void AMyPlayerController::InitializeGroupEventVoteWidget()
{
	if (!GroupEventVoteWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GroupEventVote] GroupEventVoteWidgetClass is null."));
		return;
	}

	if (!GroupEventVoteWidgetInstance)
	{
		GroupEventVoteWidgetInstance = CreateWidget<UUserWidget>(this, GroupEventVoteWidgetClass);
		if (!GroupEventVoteWidgetInstance)
		{
			UE_LOG(LogTemp, Error, TEXT("[GroupEventVote] Failed to create group event vote widget."));
			return;
		}

		GroupEventVoteWidgetInstance->AddToViewport(122);
	}

	GroupEventVoteWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	GroupEventVoteTimerText = FindGroupEventVoteText(TEXT("VoteTimer"));
	GroupEventVoteTitleText = FindGroupEventVoteText(TEXT("GroupEventVoteTitle"));
	GroupEventVoteDescriptionText = FindGroupEventVoteText(TEXT("GroupEventVoteDes"));

	VoteUserSlotWidgets.Reset();
	VoteUserSlotWidgets.Reserve(GroupEventVoteSlotCount);

	for (int32 SlotIndex = 0; SlotIndex < GroupEventVoteSlotCount; ++SlotIndex)
	{
		UVoteClickableUserWidget* VoteSlotWidget = FindVoteUserSlot(SlotIndex);
		if (!VoteSlotWidget)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[GroupEventVote] VoteUserSlot%d is missing or not reparented to UVoteClickableUserWidget."),
				SlotIndex + 1
			);
			VoteUserSlotWidgets.Add(nullptr);
			continue;
		}

		VoteSlotWidget->OnVoteClickableUserSelected.RemoveAll(this);
		VoteSlotWidget->OnVoteClickableUserSelected.AddUObject(this, &AMyPlayerController::HandleLocalGroupVoteSelection);
		VoteSlotWidget->SetCandidateVisible(false);
		VoteUserSlotWidgets.Add(VoteSlotWidget);
	}
}

void AMyPlayerController::PopulateGroupEventVoteCandidates()
{
	if (!GroupEventVoteWidgetInstance)
	{
		return;
	}

	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	TArray<APlayerState*> CandidateStates = GameState ? GameState->PlayerArray : TArray<APlayerState*>();

	int32 VisibleSlotIndex = 0;
	for (APlayerState* CandidatePlayerState : CandidateStates)
	{
		if (!CandidatePlayerState)
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

		if (!VoteUserSlotWidgets.IsValidIndex(VisibleSlotIndex))
		{
			break;
		}

		if (UVoteClickableUserWidget* VoteSlotWidget = VoteUserSlotWidgets[VisibleSlotIndex])
		{
			VoteSlotWidget->SetupCandidate(
				CandidatePlayerState->GetPlayerId(),
				ResolveDisplayedPlayerName(CandidatePlayerState)
			);
			VoteSlotWidget->SetSelected(false);
			VoteSlotWidget->SetVotingEnabled(!bHasSubmittedGroupVote);
			VoteSlotWidget->SetCandidateVisible(true);
		}

		++VisibleSlotIndex;
	}

	for (int32 SlotIndex = VisibleSlotIndex; SlotIndex < VoteUserSlotWidgets.Num(); ++SlotIndex)
	{
		if (UVoteClickableUserWidget* VoteSlotWidget = VoteUserSlotWidgets[SlotIndex])
		{
			VoteSlotWidget->SetCandidateVisible(false);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[GroupEventVote] Populated vote UI with %d candidates"), VisibleSlotIndex);
}

void AMyPlayerController::UpdateGroupEventVoteTimerDisplay()
{
	if (!GroupEventVoteTimerText)
	{
		GroupEventVoteTimerText = FindGroupEventVoteText(TEXT("VoteTimer"));
	}

	if (!GroupEventVoteTimerText)
	{
		return;
	}

	GroupEventVoteTimerText->SetText(
		GroupEventVoteRemainingSeconds > 0
			? FText::AsNumber(GroupEventVoteRemainingSeconds)
			: FText::GetEmpty()
	);
}

void AMyPlayerController::TickGroupEventVoteCountdown()
{
	GroupEventVoteRemainingSeconds = FMath::Max(0, GroupEventVoteRemainingSeconds - 1);
	UpdateGroupEventVoteTimerDisplay();

	if (GroupEventVoteRemainingSeconds <= 0)
	{
		GetWorldTimerManager().ClearTimer(GroupEventVoteCountdownHandle);
	}
}

void AMyPlayerController::HandleLocalGroupVoteSelection(int32 TargetPlayerId)
{
	if (bHasSubmittedGroupVote || ActiveGroupVoteEventID.IsNone())
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

	Server_SubmitGroupVote(ActiveGroupVoteEventID, TargetPlayerId);
}

void AMyPlayerController::DisableGroupVoteInteractions()
{
	for (UVoteClickableUserWidget* VoteSlotWidget : VoteUserSlotWidgets)
	{
		if (VoteSlotWidget)
		{
			VoteSlotWidget->SetVotingEnabled(false);
		}
	}
}

void AMyPlayerController::CloseGroupEventVoteWidget()
{
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
	SetInputMode(InputMode);

	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
}

void AMyPlayerController::ShowInspectMaliceSelectionWidget()
{
	if (!IsLocalController())
	{
		UE_LOG(LogTemp, Warning, TEXT("[InspectMalice] Ignored widget open on non-local controller: %s"), *GetName());
		return;
	}

	InitializeInspectMaliceWidget();
	if (!InspectMaliceWidgetInstance)
	{
		return;
	}

	if (UWorld *World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InspectMaliceHideTimerHandle);
	}

	PopulateInspectMaliceCandidates();
	ResetInspectMaliceSelectionWidget();
	InspectMaliceWidgetInstance->SetVisibility(ESlateVisibility::Visible);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(InspectMaliceWidgetInstance->TakeWidget());
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	UE_LOG(LogTemp, Log, TEXT("[InspectMalice] Selection widget opened."));
}

void AMyPlayerController::Client_ShowInspectMaliceSelectionWidget_Implementation()
{
	ShowInspectMaliceSelectionWidget();
}

void AMyPlayerController::PopulateInspectMaliceCandidates()
{
	if (!InspectMaliceWidgetInstance)
	{
		return;
	}

	InspectMaliceCandidatePlayerIds.Init(INDEX_NONE, InspectMaliceSlotCount);

	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	APlayerState* LocalPlayerState = GetPlayerState<APlayerState>();
	int32 VisibleSlotIndex = 0;

	if (GameState)
	{
		for (APlayerState* CandidatePlayerState : GameState->PlayerArray)
		{
			if (!CandidatePlayerState || CandidatePlayerState == LocalPlayerState)
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

			if (VisibleSlotIndex >= InspectMaliceMaxOtherPlayers || VisibleSlotIndex >= InspectMaliceSlotCount)
			{
				break;
			}

			const int32 DisplaySlotNumber = VisibleSlotIndex + 1;
			const FName ButtonName(*FString::Printf(TEXT("UserBtn%d"), DisplaySlotNumber));
			const FName TextName(*FString::Printf(TEXT("UserText%d"), DisplaySlotNumber));

			if (UButton* CandidateButton = FindInspectMaliceButton(ButtonName))
			{
				CandidateButton->SetVisibility(ESlateVisibility::Visible);
			}

			if (UTextBlock* CandidateText = FindInspectMaliceText(TextName))
			{
				CandidateText->SetText(FText::FromString(ResolveDisplayedPlayerName(CandidatePlayerState)));
				CandidateText->SetVisibility(ESlateVisibility::Visible);
			}

			InspectMaliceCandidatePlayerIds[VisibleSlotIndex] = CandidatePlayerState->GetPlayerId();
			++VisibleSlotIndex;
		}
	}

	for (int32 SlotIndex = VisibleSlotIndex; SlotIndex < InspectMaliceSlotCount; ++SlotIndex)
	{
		const FName ButtonName(*FString::Printf(TEXT("UserBtn%d"), SlotIndex + 1));
		const FName TextName(*FString::Printf(TEXT("UserText%d"), SlotIndex + 1));

		if (UButton* CandidateButton = FindInspectMaliceButton(ButtonName))
		{
			CandidateButton->SetVisibility(ESlateVisibility::Collapsed);
		}

		if (UTextBlock* CandidateText = FindInspectMaliceText(TextName))
		{
			CandidateText->SetText(FText::GetEmpty());
			CandidateText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[InspectMalice] Populated candidate list with %d players."), VisibleSlotIndex);
}

void AMyPlayerController::ResetInspectMaliceSelectionWidget()
{
	if (UTextBlock *UserText = FindInspectMaliceText(TEXT("InspectMaliceUserText")))
	{
		UserText->SetText(FText::GetEmpty());
	}

	if (UTextBlock *MaliceNumText = FindInspectMaliceText(TEXT("InspectMaliceUserMaliceNum")))
	{
		MaliceNumText->SetText(FText::GetEmpty());
	}

	SetInspectMaliceResultVisible(false);
}

void AMyPlayerController::SetInspectMaliceResultVisible(bool bVisible)
{
	UTextBlock *UserText = FindInspectMaliceText(TEXT("InspectMaliceUserText"));
	if (!UserText)
	{
		return;
	}

	if (UPanelWidget *ResultRow = UserText->GetParent())
	{
		ResultRow->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		return;
	}

	UserText->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (UTextBlock *MaliceNumText = FindInspectMaliceText(TEXT("InspectMaliceUserMaliceNum")))
	{
		MaliceNumText->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void AMyPlayerController::HideInspectMaliceSelectionWidget()
{
	if (InspectMaliceWidgetInstance)
	{
		InspectMaliceWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);

	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
}

void AMyPlayerController::SetPublicMaliceAnnouncementVisible(bool bVisible)
{
	if (UWidget *PublicMaliceWidget = FindPublicMaliceAnnouncementWidget())
	{
		PublicMaliceWidget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[PC] Public malice border widget not found. Expected name: PublicMaliceBorder"));
}

void AMyPlayerController::HidePublicMaliceAnnouncement()
{
	SetPublicMaliceAnnouncementVisible(false);
}

int32 AMyPlayerController::ResolveInspectMaliceTargetPlayerId(int32 CandidateIndex) const
{
	return InspectMaliceCandidatePlayerIds.IsValidIndex(CandidateIndex)
		? InspectMaliceCandidatePlayerIds[CandidateIndex]
		: INDEX_NONE;
}

int32 AMyPlayerController::ResolveInspectMaliceCountByPlayerId(int32 PlayerId) const
{
	if (PlayerId == INDEX_NONE)
	{
		return 0;
	}

	TArray<AActor*> FoundCharacters;
	UGameplayStatics::GetAllActorsOfClass(this, AMyCharacter::StaticClass(), FoundCharacters);
	for (AActor* FoundActor : FoundCharacters)
	{
		const AMyCharacter* CandidateCharacter = Cast<AMyCharacter>(FoundActor);
		const APlayerState* CandidatePlayerState = CandidateCharacter ? CandidateCharacter->GetPlayerState<APlayerState>() : nullptr;
		if (!CandidatePlayerState || CandidatePlayerState->GetPlayerId() != PlayerId)
		{
			continue;
		}

		const UMaliceComponent* MaliceComponent = CandidateCharacter->FindComponentByClass<UMaliceComponent>();
		return MaliceComponent ? FMath::Max(0, MaliceComponent->MaliceCount) : 0;
	}

	UE_LOG(LogTemp, Warning, TEXT("[InspectMalice] Failed to resolve malice target for PlayerId=%d"), PlayerId);
	return 0;
}

void AMyPlayerController::HandleInspectMaliceSelection(int32 CandidateIndex)
{
	const int32 TargetPlayerId = ResolveInspectMaliceTargetPlayerId(CandidateIndex);
	if (TargetPlayerId == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InspectMalice] Invalid candidate index selected: %d"), CandidateIndex);
		return;
	}

	FString TargetPlayerName = FString::Printf(TEXT("Player %d"), TargetPlayerId);
	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (GameState)
	{
		for (APlayerState* CandidatePlayerState : GameState->PlayerArray)
		{
			if (CandidatePlayerState && CandidatePlayerState->GetPlayerId() == TargetPlayerId)
			{
				TargetPlayerName = ResolveDisplayedPlayerName(CandidatePlayerState);
				break;
			}
		}
	}

	if (UTextBlock* UserText = FindInspectMaliceText(TEXT("InspectMaliceUserText")))
	{
		UserText->SetText(FText::FromString(TargetPlayerName));
	}

	if (UTextBlock* MaliceNumText = FindInspectMaliceText(TEXT("InspectMaliceUserMaliceNum")))
	{
		MaliceNumText->SetText(FText::AsNumber(ResolveInspectMaliceCountByPlayerId(TargetPlayerId)));
	}

	SetInspectMaliceResultVisible(true);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			InspectMaliceHideTimerHandle,
			this,
			&AMyPlayerController::HideInspectMaliceSelectionWidget,
			3.0f,
			false
		);
	}
}

void AMyPlayerController::OnInspectMaliceUser1Clicked()
{
	HandleInspectMaliceSelection(0);
}

void AMyPlayerController::OnInspectMaliceUser2Clicked()
{
	HandleInspectMaliceSelection(1);
}

void AMyPlayerController::OnInspectMaliceUser3Clicked()
{
	HandleInspectMaliceSelection(2);
}

void AMyPlayerController::OnInspectMaliceUser4Clicked()
{
	HandleInspectMaliceSelection(3);
}

void AMyPlayerController::OnInspectMaliceUser5Clicked()
{
	HandleInspectMaliceSelection(4);
}

void AMyPlayerController::OnInspectMaliceUser6Clicked()
{
	HandleInspectMaliceSelection(5);
}

FString AMyPlayerController::ResolveDisplayedPlayerName(const APlayerState* InPlayerState) const
{
	if (!InPlayerState)
	{
		return TEXT("Unknown");
	}

	FString PlayerName = InPlayerState->GetPlayerName();
	if (PlayerName.IsEmpty())
	{
		PlayerName = InPlayerState->GetName();
	}

	if (PlayerName.IsEmpty())
	{
		PlayerName = FString::Printf(TEXT("Player %d"), InPlayerState->GetPlayerId());
	}

	return PlayerName;
}

void AMyPlayerController::Client_OpenGroupEventVote_Implementation(
	FName EventID,
	const FText& EventTitle,
	const FText& EventDescription,
	float VoteDuration
)
{
	InitializeGroupEventVoteWidget();
	if (!GroupEventVoteWidgetInstance)
	{
		return;
	}

	FlushPressedKeys();
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
	UE_LOG(LogTemp, Log, TEXT("[GroupEventVote] Vote widget opened. event=%s duration=%d"), *EventID.ToString(), GroupEventVoteRemainingSeconds);

	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(GroupEventVoteWidgetInstance->TakeWidget());
	SetInputMode(InputMode);

	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			GroupEventVoteCountdownHandle,
			this,
			&AMyPlayerController::TickGroupEventVoteCountdown,
			1.0f,
			true
		);
	}
}

void AMyPlayerController::Client_FinishGroupEventVote_Implementation(FName EventID, const FText& ResultText)
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
			&AMyPlayerController::CloseGroupEventVoteWidget,
			2.5f,
			false
		);
	}
	else
	{
		CloseGroupEventVoteWidget();
	}
}

void AMyPlayerController::Client_ApplyConfiscationToLocalInventory_Implementation()
{
	AMyCharacter* MyCharacter = Cast<AMyCharacter>(GetPawn());
	if (!MyCharacter)
	{
		return;
	}

	if (UItemManagerComponent* ItemManager = MyCharacter->FindComponentByClass<UItemManagerComponent>())
	{
		ItemManager->RemoveAllItems();
	}
}

void AMyPlayerController::Server_SubmitGroupVote_Implementation(FName EventID, int32 TargetPlayerId)
{
	if (!ActiveGroupEvent || ActiveGroupEvent->EventID != EventID)
	{
		return;
	}

	ActiveGroupEvent->SubmitVote(this, TargetPlayerId);
}

void AMyPlayerController::Client_ShowPersonalEvent_Implementation(FName EventID, const FText& EventTitle, const FText& EventDescription, bool bIsCancelable)
{
	// 1. 함수 도달 여부 확인 (RPC가 정상적으로 클라이언트에 도착했는가?)
	UE_LOG(LogTemp, Warning, TEXT("[UI] Client_ShowPersonalEvent 도착! EventID: %s"), *EventID.ToString());

	this->FlushPressedKeys();

	// 2. 위젯 클래스 할당 여부 체크 (UI 안 뜨는 버그의 1순위 용의자 🚩)
	if (!PersonalEventWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[UI] 에러: PersonalEventWidgetClass가 비어있습니다! BP_MyPlayerController에서 클래스를 할당해주세요."));
		return;
	}

	// 3. 위젯 생성
	if (!PersonalEventWidgetInstance)
	{
		PersonalEventWidgetInstance = CreateWidget<UPersonalEventWidget>(this, PersonalEventWidgetClass);
		UE_LOG(LogTemp, Warning, TEXT("[UI] 위젯 인스턴스 생성 완료."));
	}

	if (PersonalEventWidgetInstance)
	{
		PersonalEventWidgetInstance->SetupEventUI(EventID, EventTitle, EventDescription, bIsCancelable);
        
		if (!PersonalEventWidgetInstance->IsInViewport())
		{
			PersonalEventWidgetInstance->AddToViewport(120);
			UE_LOG(LogTemp, Warning, TEXT("[UI] 위젯 AddToViewport 완료!"));
		}

		// 4. 확실하게 화면에 보이도록 강제 (투명화 버그 방지)
		PersonalEventWidgetInstance->SetVisibility(ESlateVisibility::Visible);

		// 5. 마우스 커서 표시 및 UI 조작 전용 모드로 변경
		bShowMouseCursor = true;
       
		FInputModeUIOnly InputMode;
		// 위젯에 포커스를 맞춰주어야 클릭이 씹히지 않습니다.
		InputMode.SetWidgetToFocus(PersonalEventWidgetInstance->TakeWidget()); 
		SetInputMode(InputMode);

		UE_LOG(LogTemp, Warning, TEXT("[UI] 마우스 커서 표시 및 UI 입력 모드 전환 완료!"));
	}
}

void AMyPlayerController::Server_ResolvePersonalEvent_Implementation(FName EventID, bool bIsConfirmed)
{
	// 1. 플레이어 캐릭터 확인
	AMyCharacter* MyChar = Cast<AMyCharacter>(GetPawn());
	if (!MyChar) return;

	// 2. 현재 서버가 기억하고 있는 이벤트가 있고, ID가 일치하는지 검증 (보안)
	if (ActivePersonalEvent && ActivePersonalEvent->EventID == EventID)
	{
		// 3. 해당 이벤트의 결과(보상 지급 등)를 실행!
		ActivePersonalEvent->OnEventResolved(MyChar, bIsConfirmed);
        
		// 4. 처리 완료 후 초기화
		ActivePersonalEvent = nullptr;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server] 이벤트 ID 불일치 또는 유효한 이벤트가 없습니다. 해킹 시도 의심!"));
	}
}
