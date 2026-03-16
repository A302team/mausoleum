#include "Character/MyPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Character/Components/ItemManagerComponent.h"
#include "Character/Components/MaliceComponent.h"
#include "Character/MyCharacter.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "EngineUtils.h"
#include "EnhancedInputSubsystems.h"
#include "GamePlay/Events/BaseEvent.h"
#include "GamePlay/Events/GroupEvents/BaseGroupEvent.h"
#include "GameMode/A302PlayerState.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"
#include "InputMappingContext.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameMode/A302GameInstance.h"
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
	constexpr int32 MaxInspectMaliceTargets = 5;
	constexpr int32 GroupEventVoteSlotCount = 6;

	void GatherWidgetTree(UWidget* RootWidget, TArray<UWidget*>& OutWidgets)
	{
		if (!RootWidget)
		{
			return;
		}

		OutWidgets.Add(RootWidget);

		if (UPanelWidget* PanelWidget = Cast<UPanelWidget>(RootWidget))
		{
			const int32 ChildCount = PanelWidget->GetChildrenCount();
			for (int32 ChildIndex = 0; ChildIndex < ChildCount; ++ChildIndex)
			{
				GatherWidgetTree(PanelWidget->GetChildAt(ChildIndex), OutWidgets);
			}
		}
	}

	template <typename WidgetType>
	WidgetType* FindNamedWidget(UUserWidget* RootWidget, std::initializer_list<const TCHAR*> PreferredNames)
	{
		if (!RootWidget)
		{
			return nullptr;
		}

		for (const TCHAR* PreferredName : PreferredNames)
		{
			if (WidgetType* FoundWidget = Cast<WidgetType>(RootWidget->GetWidgetFromName(FName(PreferredName))))
			{
				return FoundWidget;
			}
		}

		return nullptr;
	}

	AMyCharacter *FindCharacterForPlayerState(const UObject *WorldContextObject, const APlayerState *TargetPlayerState)
	{
		if (!WorldContextObject || !TargetPlayerState)
		{
			return nullptr;
		}

		UWorld *World = WorldContextObject->GetWorld();
		if (!World)
		{
			return nullptr;
		}

		for (TActorIterator<AMyCharacter> It(World); It; ++It)
		{
			if (It->GetPlayerState() == TargetPlayerState)
			{
				return *It;
			}
		}

		return nullptr;
	}
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

UButton *AMyPlayerController::FindInspectMaliceButton(const FName &WidgetName) const
{
	if (!InspectMaliceWidgetInstance)
	{
		return nullptr;
	}

	return Cast<UButton>(InspectMaliceWidgetInstance->GetWidgetFromName(WidgetName));
}

UTextBlock *AMyPlayerController::FindInspectMaliceText(const FName &WidgetName) const
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

UTextBlock *AMyPlayerController::FindPublicMaliceAnnouncementText(const FName &WidgetName) const
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

UTextBlock* AMyPlayerController::FindTitleCardText(const FName& WidgetName) const
{
	if (!TitleCardWidgetInstance)
	{
		return nullptr;
	}

	return Cast<UTextBlock>(TitleCardWidgetInstance->GetWidgetFromName(WidgetName));
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

void AMyPlayerController::HideTitleCard()
{
	if (TitleCardWidgetInstance)
	{
		TitleCardWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void AMyPlayerController::Client_HideTitleCard_Implementation()
{
	HideTitleCard();
}

bool AMyPlayerController::IsInGameSettingMenuOpen() const
{
	return InGameSettingWidget && InGameSettingWidget->GetVisibility() == ESlateVisibility::Visible;
}

float AMyPlayerController::GetMouseSensitivityMultiplier() const
{
	return MouseSensitivityMultiplier;
}

void AMyPlayerController::ShowPublicMaliceAnnouncement(const FString &PlayerName, int32 MaliceCount)
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
			false);
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

	ResolutionComboBox = FindNamedWidget<UComboBoxString>(InGameSettingWidget, { TEXT("ResolutionComboBox"), TEXT("VideoResolutionComboBox") });
	FullscreenModeComboBox = FindNamedWidget<UComboBoxString>(InGameSettingWidget, { TEXT("FullscreenModeComboBox"), TEXT("ScreenModeComboBox") });
	FrameLimitComboBox = FindNamedWidget<UComboBoxString>(InGameSettingWidget, { TEXT("FrameLimitComboBox"), TEXT("FrameRateComboBox"), TEXT("FPSComboBox") });
	ResolutionApplyBtn = FindNamedWidget<UButton>(InGameSettingWidget, { TEXT("ApplyBtn"), TEXT("ResolutionApplyBtn") });
	ExitBtn = FindNamedWidget<UButton>(InGameSettingWidget, { TEXT("ExitBtn") });
	VSyncCheckBox = FindNamedWidget<UCheckBox>(InGameSettingWidget, { TEXT("VSyncCheckBox"), TEXT("VerticalSyncCheckBox") });
	MouseSensitivitySlider = FindNamedWidget<USlider>(InGameSettingWidget, { TEXT("Mouse_Slider"), TEXT("MouseSensitivitySlider") });
	MasterVolumeSlider = FindNamedWidget<USlider>(InGameSettingWidget, { TEXT("MasterVolume_Slider") });
	BGMVolumeSlider = FindNamedWidget<USlider>(InGameSettingWidget, { TEXT("BGM_Slider") });
	SFXVolumeSlider = FindNamedWidget<USlider>(InGameSettingWidget, { TEXT("SFX_Slider") });
	InterfaceVolumeSlider = FindNamedWidget<USlider>(InGameSettingWidget, { TEXT("Interface_Slider") });

	if (!ResolutionComboBox)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] ResolutionComboBox widget not found. Expected explicit name: ResolutionComboBox (or VideoResolutionComboBox)."));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[PC] Bound settings resolution combo: %s"), *ResolutionComboBox->GetName());
	}

	if (!FullscreenModeComboBox)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] FullscreenModeComboBox widget not found. Expected explicit name: FullscreenModeComboBox (or ScreenModeComboBox)."));
	}

	if (!FrameLimitComboBox)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] FrameLimitComboBox widget not found. Expected explicit name: FrameLimitComboBox (or FrameRateComboBox / FPSComboBox)."));
	}

	if (!VSyncCheckBox)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] VSyncCheckBox widget not found. Expected explicit name: VSyncCheckBox (or VerticalSyncCheckBox)."));
	}

	if (!MouseSensitivitySlider)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] MouseSensitivitySlider widget not found. Expected explicit name: Mouse_Slider (or MouseSensitivitySlider)."));
	}
	else
	{
		MouseSensitivitySlider->OnValueChanged.RemoveDynamic(this, &AMyPlayerController::OnMouseSensitivitySliderChanged);
		MouseSensitivitySlider->OnValueChanged.AddDynamic(this, &AMyPlayerController::OnMouseSensitivitySliderChanged);
	}

	if (!MasterVolumeSlider)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] MasterVolume slider widget not found. Expected explicit name: MasterVolume_Slider."));
	}

	if (!BGMVolumeSlider)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] BGM slider widget not found. Expected explicit name: BGM_Slider."));
	}

	if (!SFXVolumeSlider)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] SFX slider widget not found. Expected explicit name: SFX_Slider."));
	}

	if (!InterfaceVolumeSlider)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] Interface slider widget not found. Expected explicit name: Interface_Slider."));
	}

	if (ResolutionApplyBtn)
	{
		ResolutionApplyBtn->OnClicked.RemoveDynamic(this, &AMyPlayerController::OnResolutionApplyClicked);
		ResolutionApplyBtn->OnClicked.AddDynamic(this, &AMyPlayerController::OnResolutionApplyClicked);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] ResolutionApplyBtn widget not found. Expected ResolutionApplyBtn or ApplyBtn."));
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

	EnsureVideoSettingOptions();
	SyncVideoSettingsToCurrent();
	SyncMouseSensitivitySliderToCurrent();
	SyncAudioSlidersToCurrent();
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

	SyncVideoSettingsToCurrent();
	SyncMouseSensitivitySliderToCurrent();
	SyncAudioSlidersToCurrent();
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

void AMyPlayerController::Server_RegisterPlayerDisplayName_Implementation(const FString &DesiredName)
{
	if (!PlayerState)
	{
		return;
	}

	const FString TrimmedName = DesiredName.TrimStartAndEnd();
	if (TrimmedName.IsEmpty())
	{
		return;
	}

	PlayerState->SetPlayerName(TrimmedName.Left(32));
}

AMyPlayerController::AMyPlayerController()
{
	LoadMouseSensitivitySetting();
	LoadAudioSettings();

	static ConstructorHelpers::FClassFinder<UUserWidget> InGameSettingWidgetBPClass(TEXT("/Game/WorkSpace/UI/WBP_InGameSetting2"));
	if (InGameSettingWidgetBPClass.Succeeded())
	{
		InGameSettingClass = InGameSettingWidgetBPClass.Class;
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> TitleCardWidgetBPClass(TEXT("/Game/WorkSpace/UI/WBP_TitleCard"));
	if (TitleCardWidgetBPClass.Succeeded())
	{
		TitleCardWidgetClass = TitleCardWidgetBPClass.Class;
	}

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

	SetInputMode(FInputModeGameOnly());

	if (const UA302GameInstance *GameInstance = Cast<UA302GameInstance>(GetGameInstance()))
	{
		if (!GameInstance->MyPlayerName.IsEmpty())
		{
			Server_RegisterPlayerDisplayName(GameInstance->MyPlayerName);
		}
	}

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

	AGameModeBase* ActiveGameMode = UGameplayStatics::GetGameMode(this);
	AA302GameMode *GameMode = Cast<AA302GameMode>(ActiveGameMode);
	const bool bUsesMyGameMode = ActiveGameMode && ActiveGameMode->GetClass()->GetName().Contains(TEXT("MyGameMode"));
	if ((GameMode || bUsesMyGameMode) && !HUDWidget)
	{
		TSubclassOf<UUserWidget> HUDClassToCreate = nullptr;
		if (GameMode && GameMode->HUDWidgetClass)
		{
			HUDClassToCreate = GameMode->HUDWidgetClass;
		}
		else if (bUsesMyGameMode)
		{
			HUDClassToCreate = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WorkSpace/UI/WBP_HUD2.WBP_HUD2_C"));
		}

		if (HUDClassToCreate)
		{
			HUDWidget = CreateWidget<UUserWidget>(this, HUDClassToCreate);
			if (HUDWidget)
			{
				HUDWidget->AddToViewport();
				QuickSlotBarWidget = HUDWidget;
				InitializeQuickSlotVisualState();
				UpdateShieldCountText(0);
				UpdateMaliceCountText(0);
				UpdateItemTimerText(30.0f);
				SetItemTimerVisible(false);
				UE_LOG(LogTemp, Log, TEXT("[PC] HUDWidget 생성 성공: %s"), *GetNameSafe(HUDClassToCreate));
			}
		}

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

	if (!QuickSlotBarWidget && QuickSlotBarClass)
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
	else if (!QuickSlotBarWidget)
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

	if (UButton* Player1Button = FindInspectMaliceButton(TEXT("UserBtn1")))
	{
		Player1Button->OnClicked.RemoveDynamic(this, &AMyPlayerController::OnInspectMalicePlayer1Clicked);
		Player1Button->OnClicked.AddDynamic(this, &AMyPlayerController::OnInspectMalicePlayer1Clicked);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[InspectMalice] UserBtn1 widget not found."));
	}

	if (UButton* Player2Button = FindInspectMaliceButton(TEXT("UserBtn2")))
	{
		Player2Button->OnClicked.RemoveDynamic(this, &AMyPlayerController::OnInspectMalicePlayer2Clicked);
		Player2Button->OnClicked.AddDynamic(this, &AMyPlayerController::OnInspectMalicePlayer2Clicked);
	}

	if (UButton* Player3Button = FindInspectMaliceButton(TEXT("UserBtn3")))
	{
		Player3Button->OnClicked.RemoveDynamic(this, &AMyPlayerController::OnInspectMalicePlayer3Clicked);
		Player3Button->OnClicked.AddDynamic(this, &AMyPlayerController::OnInspectMalicePlayer3Clicked);
	}

	if (UButton* Player4Button = FindInspectMaliceButton(TEXT("UserBtn4")))
	{
		Player4Button->OnClicked.RemoveDynamic(this, &AMyPlayerController::OnInspectMalicePlayer4Clicked);
		Player4Button->OnClicked.AddDynamic(this, &AMyPlayerController::OnInspectMalicePlayer4Clicked);
	}

	if (UButton* Player5Button = FindInspectMaliceButton(TEXT("UserBtn5")))
	{
		Player5Button->OnClicked.RemoveDynamic(this, &AMyPlayerController::OnInspectMalicePlayer5Clicked);
		Player5Button->OnClicked.AddDynamic(this, &AMyPlayerController::OnInspectMalicePlayer5Clicked);
	}

	for (int32 Index = 1; Index <= GroupEventVoteSlotCount; ++Index)
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
				ResolvePlayerDisplayName(CandidatePlayerState)
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

	ResetInspectMaliceSelectionWidget();
	PopulateInspectMaliceSelectionWidget();
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

void AMyPlayerController::PopulateInspectMaliceSelectionWidget()
{
	InspectMaliceSelectablePlayers.Reset();

	AGameStateBase *GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GameState)
	{
		return;
	}

	APlayerState *LocalPlayerState = PlayerState;
	for (APlayerState *CandidatePlayerState : GameState->PlayerArray)
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

		if (InspectMaliceSelectablePlayers.Num() >= MaxInspectMaliceTargets)
		{
			break;
		}

		InspectMaliceSelectablePlayers.Add(CandidatePlayerState);

		const int32 WidgetIndex = InspectMaliceSelectablePlayers.Num();
		const FName ButtonName(*FString::Printf(TEXT("UserBtn%d"), WidgetIndex));
		if (UButton *TargetButton = FindInspectMaliceButton(ButtonName))
		{
			TargetButton->SetVisibility(ESlateVisibility::Visible);
			TargetButton->SetIsEnabled(true);
		}

		const FName TextName(*FString::Printf(TEXT("UserText%d"), WidgetIndex));
		if (UTextBlock *TargetText = FindInspectMaliceText(TextName))
		{
			TargetText->SetText(FText::FromString(ResolvePlayerDisplayName(CandidatePlayerState)));
			TargetText->SetVisibility(ESlateVisibility::Visible);
		}
	}

	for (int32 WidgetIndex = InspectMaliceSelectablePlayers.Num() + 1; WidgetIndex <= 6; ++WidgetIndex)
	{
		const FName ButtonName(*FString::Printf(TEXT("UserBtn%d"), WidgetIndex));
		if (UButton *HiddenButton = FindInspectMaliceButton(ButtonName))
		{
			HiddenButton->SetVisibility(ESlateVisibility::Collapsed);
			HiddenButton->SetIsEnabled(false);
		}

		const FName TextName(*FString::Printf(TEXT("UserText%d"), WidgetIndex));
		if (UTextBlock *HiddenText = FindInspectMaliceText(TextName))
		{
			HiddenText->SetVisibility(ESlateVisibility::Collapsed);
			HiddenText->SetText(FText::GetEmpty());
		}
	}
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

int32 AMyPlayerController::QueryPlayerMaliceCount(const APlayerState *TargetPlayerState) const
{
	const AMyCharacter *TargetCharacter = FindCharacterForPlayerState(this, TargetPlayerState);
	if (!TargetCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InspectMalice] Character lookup failed. Returning 0 malice."));
		return 0;
	}

	const UMaliceComponent *MaliceComponent = TargetCharacter->FindComponentByClass<UMaliceComponent>();
	return MaliceComponent ? MaliceComponent->GetMaliceCount() : 0;
}

FString AMyPlayerController::ResolvePlayerDisplayName(const APlayerState *TargetPlayerState) const
{
	if (!TargetPlayerState)
	{
		return TEXT("Unknown");
	}

	const FString PlayerName = TargetPlayerState->GetPlayerName();
	return PlayerName.IsEmpty() ? GetNameSafe(TargetPlayerState) : PlayerName;
}

void AMyPlayerController::ApplyInspectMaliceSelection(int32 EntryIndex)
{
	if (!InspectMaliceSelectablePlayers.IsValidIndex(EntryIndex))
	{
		return;
	}

	APlayerState *TargetPlayerState = InspectMaliceSelectablePlayers[EntryIndex];
	if (!TargetPlayerState)
	{
		return;
	}

	if (UTextBlock *UserText = FindInspectMaliceText(TEXT("InspectMaliceUserText")))
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
			&AMyPlayerController::HideInspectMaliceSelectionWidget,
			3.0f,
			false);
	}
}

void AMyPlayerController::OnInspectMalicePlayer1Clicked()
{
	ApplyInspectMaliceSelection(0);
}

void AMyPlayerController::OnInspectMalicePlayer2Clicked()
{
	ApplyInspectMaliceSelection(1);
}

void AMyPlayerController::OnInspectMalicePlayer3Clicked()
{
	ApplyInspectMaliceSelection(2);
}

void AMyPlayerController::OnInspectMalicePlayer4Clicked()
{
	ApplyInspectMaliceSelection(3);
}

void AMyPlayerController::OnInspectMalicePlayer5Clicked()
{
	ApplyInspectMaliceSelection(4);
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

void AMyPlayerController::Client_ShowPersonalEvent_Implementation(FName EventID, const FText& EventTitle, const FText& EventDescription, const TArray<FText>& Choices)
{
	FlushPressedKeys();

	if (!PersonalEventWidgetClass)
	{
		return;
	}

	if (!PersonalEventWidgetInstance)
	{
		PersonalEventWidgetInstance = CreateWidget<UPersonalEventWidget>(this, PersonalEventWidgetClass);
	}

	if (PersonalEventWidgetInstance)
	{
		PersonalEventWidgetInstance->SetupEventUI(EventID, EventTitle, EventDescription, Choices);

		if (!PersonalEventWidgetInstance->IsInViewport())
		{
			PersonalEventWidgetInstance->AddToViewport(120);
		}

		PersonalEventWidgetInstance->SetVisibility(ESlateVisibility::Visible);

		// 마우스 커서 표시 및 UI 조작 전용 모드로 변경
		bShowMouseCursor = true;
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(PersonalEventWidgetInstance->TakeWidget());
		SetInputMode(InputMode);
	}
}

void AMyPlayerController::Client_ShowTitleCard_Implementation(const FText& Title, const FText& Context, float DisplaySeconds)
{
	if (!TitleCardWidgetClass)
	{
		return;
	}

	if (!TitleCardWidgetInstance)
	{
		TitleCardWidgetInstance = CreateWidget<UUserWidget>(this, TitleCardWidgetClass);
	}

	if (!TitleCardWidgetInstance)
	{
		return;
	}

	if (UTextBlock* TitleText = FindTitleCardText(TEXT("EventTitle")))
	{
		TitleText->SetText(Title);
	}

	if (UTextBlock* ContextText = FindTitleCardText(TEXT("EventContext")))
	{
		ContextText->SetText(Context);
	}

	if (!TitleCardWidgetInstance->IsInViewport())
	{
		TitleCardWidgetInstance->AddToViewport(140);
	}

	TitleCardWidgetInstance->SetVisibility(ESlateVisibility::Visible);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TitleCardHideTimerHandle);

		if (DisplaySeconds > 0.0f)
		{
			World->GetTimerManager().SetTimer(
				TitleCardHideTimerHandle,
				this,
				&AMyPlayerController::HideTitleCard,
				FMath::Max(0.5f, DisplaySeconds),
				false
			);
		}
	}
}

void AMyPlayerController::Server_ResolvePersonalEvent_Implementation(FName EventID, int32 ChoiceIndex)
{
	AMyCharacter* MyChar = Cast<AMyCharacter>(GetPawn());
	if (!MyChar)
	{
		return;
	}

	// 만약 플레이어가 취소를 눌렀다면 여기서 조기 종료
	if (ChoiceIndex == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Event] %s 거절됨."), *EventID.ToString());
	}

	if (UBasePersonalEvent *TargetEvent = Cast<UBasePersonalEvent>(ActivePersonalEvent))
	{
		if (TargetEvent->EventID == EventID)
		{
			TargetEvent->OnEventResolvedMulti(MyChar, ChoiceIndex);
		}
	}
	// 이벤트 캐시 초기화
	ActivePersonalEvent = nullptr;
}
