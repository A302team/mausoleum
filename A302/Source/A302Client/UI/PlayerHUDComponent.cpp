#include "UI/PlayerHUDComponent.h"

#include "Character/Components/MaliceComponent.h"
#include "Character/Components/Inventory/QuickSlotComponent.h"
#include "Character/MyPlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameData/Items/ItemDefinition.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerState.h"
#include "GameMode/A302PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "UI/GroupEventHUDComponent.h"
#include "UI/PersonalEventWidget.h"
#include "Room/RoomScopeRules.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

namespace
{
	constexpr int32 PlayerHUDVoteSlotCount = 6;
	constexpr int32 PlayerHUDQuickSlotCount = 6;
	constexpr int32 PlayerHUDRequiredQuickSlotCount = 5;
	constexpr int32 MaxInspectMaliceTargets = 5;
	constexpr int32 MaxNicknameUiLen = 7;
	constexpr float DefaultMouseSensitivityMultiplier = 1.0f;
	constexpr float MinMouseSensitivityMultiplier = 0.1f;
	constexpr float MaxMouseSensitivityMultiplier = 3.0f;
	const TCHAR* LocalSettingsSection = TEXT("/Script/A302.LocalSettings");
	const TCHAR* MouseSensitivityConfigKey = TEXT("MouseSensitivityMultiplier");
	const TCHAR* MasterVolumeConfigKey = TEXT("MasterVolume");
	const TCHAR* BGMVolumeConfigKey = TEXT("BGMVolume");
	const TCHAR* SFXVolumeConfigKey = TEXT("SFXVolume");
	const TCHAR* InterfaceVolumeConfigKey = TEXT("InterfaceVolume");
	constexpr float DefaultInspectMaliceSelectionTimeoutSeconds = 10.0f;
	constexpr float DefaultInspectMaliceResultDisplaySeconds = 3.0f;

	FText BuildClockText(float RemainingSeconds)
	{
		const int32 SafeSeconds = FMath::Max(0, FMath::CeilToInt(RemainingSeconds));
		return FText::FromString(FString::Printf(TEXT("%d"), SafeSeconds));
	}

	FString ClampNicknameForUi(const FString& Name)
	{
		return Name.Len() > MaxNicknameUiLen ? Name.Left(MaxNicknameUiLen) : Name;
	}

	bool IsInspectMaliceCandidateInScope(const APlayerState* LocalPlayerState, const APlayerState* CandidatePlayerState)
	{
		if (!LocalPlayerState || !CandidatePlayerState)
		{
			return false;
		}

		const FString LocalRoomCode = A302RoomScope::ResolvePlayerRoomCode(LocalPlayerState);
		const FString CandidateRoomCode = A302RoomScope::ResolvePlayerRoomCode(CandidatePlayerState);
		if (!LocalRoomCode.IsEmpty() && !CandidateRoomCode.IsEmpty())
		{
			return LocalRoomCode == CandidateRoomCode;
		}

		// PIE/local tests often run without authoritative room codes.
		// In that case, fall back to the current world's player list only.
		return true;
	}

	void EnsureComboHasOption(UComboBoxString* ComboBox, const FString& Option)
	{
		if (!ComboBox || Option.IsEmpty())
		{
			return;
		}

		const int32 OptionCount = ComboBox->GetOptionCount();
		for (int32 OptionIndex = 0; OptionIndex < OptionCount; ++OptionIndex)
		{
			if (ComboBox->GetOptionAtIndex(OptionIndex).Equals(Option, ESearchCase::IgnoreCase))
			{
				return;
			}
		}

		ComboBox->AddOption(Option);
	}

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

	bool HasExpectedQuickSlotLayout(UUserWidget* RootWidget, FString* OutMissingNames = nullptr)
	{
		if (!RootWidget)
		{
			return false;
		}

		const UClass* WidgetClass = RootWidget->GetClass();
		const bool bIsHud2Layout = WidgetClass && WidgetClass->GetName().Contains(TEXT("WBP_HUD2"), ESearchCase::IgnoreCase);

		TArray<FString> MissingNames;
		// Keep backward compatibility with existing 5-slot HUDs while enabling optional slot 6.
		for (int32 SlotIndex = 0; SlotIndex < PlayerHUDRequiredQuickSlotCount; ++SlotIndex)
		{
			const FName SlotWidgetName(*FString::Printf(TEXT("QuickSlot%d"), SlotIndex + 1));
			if (!RootWidget->GetWidgetFromName(SlotWidgetName))
			{
				MissingNames.Add(SlotWidgetName.ToString());
			}
		}

		if (!bIsHud2Layout)
		{
			static const FName RequiredTextNames[] =
			{
				TEXT("ShieldCount"),
				TEXT("MaliceCount"),
				TEXT("ItemTimer")
			};

			for (const FName RequiredName : RequiredTextNames)
			{
				if (!RootWidget->GetWidgetFromName(RequiredName))
				{
					MissingNames.Add(RequiredName.ToString());
				}
			}
		}

		if (OutMissingNames)
		{
			*OutMissingNames = FString::Join(MissingNames, TEXT(","));
		}

		return MissingNames.Num() == 0;
	}
}

UPlayerHUDComponent::UPlayerHUDComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	LoadMouseSensitivitySetting();
	LoadAudioSettings();
}

AMyPlayerController* UPlayerHUDComponent::GetOwnerController() const
{
	if (AHUD* OwningHUD = Cast<AHUD>(GetOwner()))
	{
		return Cast<AMyPlayerController>(OwningHUD->PlayerOwner);
	}
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
		PersonalEventWidgetInstance->AddToViewport(150);
	}

	PersonalEventWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	OwnerController->bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(PersonalEventWidgetInstance->TakeWidget());
	OwnerController->SetInputMode(InputMode);
}

void UPlayerHUDComponent::EnsureGroupEventHUDComponent()
{
	if (GroupEventHUDComponent)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	GroupEventHUDComponent = Cast<UGroupEventHUDComponent>(OwnerActor->GetComponentByClass(UGroupEventHUDComponent::StaticClass()));
	if (!GroupEventHUDComponent)
	{
		GroupEventHUDComponent = NewObject<UGroupEventHUDComponent>(OwnerActor, TEXT("GroupEventHUDComponent"));
		if (GroupEventHUDComponent)
		{
			GroupEventHUDComponent->RegisterComponent();
		}
	}
}

void UPlayerHUDComponent::ShowGroupEventVoteUI(TSubclassOf<UUserWidget> GroupEventVoteWidgetClass, FName EventID, const FText& EventTitle, const FText& EventDescription, float VoteDuration)
{
	EnsureGroupEventHUDComponent();
	if (GroupEventHUDComponent)
	{
		GroupEventHUDComponent->ShowGroupEventVoteUI(GroupEventVoteWidgetClass, EventID, EventTitle, EventDescription, VoteDuration);
	}
}

void UPlayerHUDComponent::FinishGroupEventVoteUI(FName EventID, const FText& ResultText)
{
	EnsureGroupEventHUDComponent();
	if (GroupEventHUDComponent)
	{
		GroupEventHUDComponent->FinishGroupEventVoteUI(EventID, ResultText);
	}
}

void UPlayerHUDComponent::CloseGroupEventVoteUI()
{
	EnsureGroupEventHUDComponent();
	if (GroupEventHUDComponent)
	{
		GroupEventHUDComponent->CloseGroupEventVoteUI();
	}
}

FString UPlayerHUDComponent::ResolvePlayerDisplayName(const APlayerState* TargetPlayerState) const
{
	if (!TargetPlayerState)
	{
		return TEXT("Unknown");
	}

	const FString PlayerName = TargetPlayerState->GetPlayerName();
	return ClampNicknameForUi(PlayerName.IsEmpty() ? GetNameSafe(TargetPlayerState) : PlayerName);
}

void UPlayerHUDComponent::InitializeInGameHUD(TSubclassOf<UUserWidget> InQuickSlotBarClass, TSubclassOf<UUserWidget> InInGameSettingClass, TSubclassOf<UUserWidget> InInspectMaliceWidgetClass)
{
	QuickSlotBarClass = InQuickSlotBarClass;
	InGameSettingClass = InInGameSettingClass;
	InspectMaliceWidgetClass = InInspectMaliceWidgetClass;

	EnsureGroupEventHUDComponent();
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

float UPlayerHUDComponent::GetMouseSensitivityMultiplier() const
{
	return FMath::Max(0.01f, MouseSensitivityMultiplier);
}

void UPlayerHUDComponent::ShowPublicMaliceAnnouncement(const FString& PlayerName, int32 MaliceCount)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PublicMaliceAnnouncementHideTimerHandle);
	}

	if (UTextBlock* UserText = FindPublicMaliceAnnouncementText(TEXT("PublicMaliceBorderUser")))
	{
		UserText->SetText(FText::FromString(ClampNicknameForUi(PlayerName)));
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
	ShowInspectMaliceSelectionWidgetWithConfig(DefaultInspectMaliceSelectionTimeoutSeconds, DefaultInspectMaliceResultDisplaySeconds);
}

void UPlayerHUDComponent::ShowInspectMaliceSelectionWidgetWithConfig(float SelectionTimeoutSeconds, float ResultDisplaySeconds)
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
		World->GetTimerManager().ClearTimer(InspectMaliceSelectionTimeoutHandle);
		World->GetTimerManager().ClearTimer(InspectMalicePopulateRetryHandle);
		World->GetTimerManager().ClearTimer(InspectMaliceItemTimerTickHandle);
	}

	InspectMaliceSelectionTimeoutSeconds = FMath::Max(0.1f, SelectionTimeoutSeconds);
	InspectMaliceResultDisplaySeconds = FMath::Max(0.1f, ResultDisplaySeconds);
	bInspectMaliceSelectionConsumed = false;
	bInspectMaliceItemTimerBaselineCaptured = false;

	ResetInspectMaliceSelectionWidget();
	PopulateInspectMaliceSelectionWidget();
	SetInspectMaliceSelectionButtonsEnabled(true);
	InspectMaliceWidgetInstance->SetVisibility(ESlateVisibility::Visible);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(InspectMaliceWidgetInstance->TakeWidget());
	InputMode.SetHideCursorDuringCapture(false);
	OwnerController->SetInputMode(InputMode);

	OwnerController->bShowMouseCursor = true;
	OwnerController->bEnableClickEvents = true;
	OwnerController->bEnableMouseOverEvents = true;
	StartInspectMaliceItemTimer(InspectMaliceSelectionTimeoutSeconds);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			InspectMaliceSelectionTimeoutHandle,
			this,
			&UPlayerHUDComponent::HandleInspectMaliceSelectionTimeout,
			InspectMaliceSelectionTimeoutSeconds,
			false
		);
	}
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

void UPlayerHUDComponent::ConfigureMatchTimer(float MatchStartServerTime, float DurationSeconds, bool bVisible)
{
	MatchTimerStartServerTime = MatchStartServerTime;
	MatchTimerDurationSeconds = FMath::Max(0.0f, DurationSeconds);
	bMatchTimerVisible = bVisible && MatchTimerDurationSeconds > 0.0f;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MatchTimerTickHandle);
	}

	UTextBlock* FoundText = FindMatchTimerText();
	UE_LOG(LogTemp, Warning, TEXT("[MatchTimerDebug] ConfigureMatchTimer called. start=%f, duration=%f, visible=%d. WidgetFound=%d"), 
		MatchTimerStartServerTime, MatchTimerDurationSeconds, bVisible, FoundText != nullptr);

	if (!bMatchTimerVisible)
	{
		StopMatchTimer();
		return;
	}

	TickMatchTimer();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			MatchTimerTickHandle,
			this,
			&UPlayerHUDComponent::TickMatchTimer,
			0.25f,
			true
		);
	}
}

void UPlayerHUDComponent::InitializeQuickSlotWidget()
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController || !QuickSlotBarClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerHUDComponent] InitializeQuickSlotWidget skipped. controller=%s class=%s"), *GetNameSafe(OwnerController), *GetNameSafe(QuickSlotBarClass.Get()));
		return;
	}

	auto CreateAndAttachQuickSlotWidget = [&](TSubclassOf<UUserWidget> WidgetClass) -> UUserWidget*
	{
		if (!WidgetClass)
		{
			return nullptr;
		}

		UUserWidget* NewWidget = CreateWidget<UUserWidget>(OwnerController, WidgetClass);
		if (!NewWidget)
		{
			return nullptr;
		}

		if (!NewWidget->IsInViewport())
		{
			if (!NewWidget->AddToPlayerScreen(500))
			{
				NewWidget->AddToViewport(500);
			}
		}

		return NewWidget;
	};

	if (!QuickSlotBarWidget)
	{
		QuickSlotBarWidget = CreateAndAttachQuickSlotWidget(QuickSlotBarClass);
		if (!QuickSlotBarWidget)
		{
			UE_LOG(LogTemp, Error, TEXT("[PlayerHUDComponent] Failed to create quick slot widget. class=%s"), *GetNameSafe(QuickSlotBarClass.Get()));
			return;
		}
	}

	FString MissingNames;
	if (!HasExpectedQuickSlotLayout(QuickSlotBarWidget, &MissingNames))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[PlayerHUDComponent] Quick slot layout mismatch. class=%s missing=[%s]. Forcing WBP_HUD2."),
			*GetNameSafe(QuickSlotBarClass.Get()),
			*MissingNames
		);

		if (UClass* QuickSlotWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WorkSpace/UI/WBP_HUD2.WBP_HUD2_C")))
		{
			if (QuickSlotWidgetClass != QuickSlotBarWidget->GetClass())
			{
				if (UUserWidget* CandidateWidget = CreateAndAttachQuickSlotWidget(QuickSlotWidgetClass))
				{
					FString CandidateMissingNames;
					if (HasExpectedQuickSlotLayout(CandidateWidget, &CandidateMissingNames))
					{
						QuickSlotBarWidget->RemoveFromParent();
						QuickSlotBarWidget = CandidateWidget;
						QuickSlotBarClass = QuickSlotWidgetClass;
						UE_LOG(LogTemp, Log, TEXT("[PlayerHUDComponent] Applied direct quick slot class=%s"), *GetNameSafe(QuickSlotBarClass.Get()));
					}
					else
					{
						UE_LOG(
							LogTemp,
							Error,
							TEXT("[PlayerHUDComponent] WBP_HUD2 layout mismatch. missing=[%s]"),
							*CandidateMissingNames
						);
						CandidateWidget->RemoveFromParent();
					}
				}
			}
		}
	}

	QuickSlotBarWidget->SetVisibility(ESlateVisibility::Visible);
	QuickSlotBarWidget->SetIsEnabled(true);
	UE_LOG(LogTemp, Log, TEXT("[PlayerHUDComponent] Quick slot widget ready. class=%s widget=%s"), *GetNameSafe(QuickSlotBarClass.Get()), *GetNameSafe(QuickSlotBarWidget));

	InitializeQuickSlotVisualState();
	BindQuickSlotComponent();
	SyncQuickSlotUIFromComponent();
	UpdateShieldCountText(0);
	UpdateMaliceCountText(0);
	UpdateItemTimerText(30.0f);
	SetItemTimerVisible(false);
	if (UTextBlock* MatchTimerText = FindMatchTimerText())
	{
		MatchTimerText->SetText(FText::GetEmpty());
		MatchTimerText->SetVisibility(ESlateVisibility::Hidden);
	}
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

UTextBlock* UPlayerHUDComponent::FindMatchTimerText() const
{
	if (!QuickSlotBarWidget)
	{
		return nullptr;
	}

	static const FName CandidateNames[] =
	{
		TEXT("MatchTimer"),
		TEXT("GameTimer"),
		TEXT("StageTimer"),
		TEXT("RemainingTime"),
		TEXT("RemainTimeText"),
		TEXT("TimeLimit"),
		TEXT("TimerText")
	};

	for (const FName& CandidateName : CandidateNames)
	{
		if (UTextBlock* TextBlock = Cast<UTextBlock>(QuickSlotBarWidget->GetWidgetFromName(CandidateName)))
		{
			return TextBlock;
		}
	}

	return nullptr;
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

void UPlayerHUDComponent::TickMatchTimer()
{
	UTextBlock* MatchTimerText = FindMatchTimerText();
	if (!MatchTimerText)
	{
		return;
	}

	if (!bMatchTimerVisible || MatchTimerDurationSeconds <= 0.0f)
	{
		StopMatchTimer();
		return;
	}

	const AGameStateBase* GameStateBase = UGameplayStatics::GetGameState(this);
	const double CurrentServerTime = GameStateBase
		? static_cast<double>(GameStateBase->GetServerWorldTimeSeconds())
		: (GetWorld() ? static_cast<double>(GetWorld()->GetTimeSeconds()) : 0.0);
	const float RemainingSeconds = FMath::Max(0.0f, MatchTimerDurationSeconds - static_cast<float>(CurrentServerTime - MatchTimerStartServerTime));

	MatchTimerText->SetText(BuildClockText(RemainingSeconds));
	MatchTimerText->SetVisibility(ESlateVisibility::Visible);
}

void UPlayerHUDComponent::StopMatchTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MatchTimerTickHandle);
	}

	bMatchTimerVisible = false;
	if (UTextBlock* MatchTimerText = FindMatchTimerText())
	{
		MatchTimerText->SetVisibility(ESlateVisibility::Hidden);
	}
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
		UE_LOG(LogTemp, Warning, TEXT("[PlayerHUDComponent] InitializeInGameSettingWidget skipped. controller=%s class=%s"), *GetNameSafe(OwnerController), *GetNameSafe(InGameSettingClass.Get()));
		return;
	}

	if (!InGameSettingWidget)
	{
		InGameSettingWidget = CreateWidget<UUserWidget>(OwnerController, InGameSettingClass);
		if (!InGameSettingWidget)
		{
			UE_LOG(LogTemp, Error, TEXT("[PlayerHUDComponent] Failed to create in-game setting widget. class=%s"), *GetNameSafe(InGameSettingClass.Get()));
			return;
		}

		if (!InGameSettingWidget->IsInViewport())
		{
			if (!InGameSettingWidget->AddToPlayerScreen(510))
			{
				InGameSettingWidget->AddToViewport(510);
			}
		}
		InGameSettingWidget->SetVisibility(ESlateVisibility::Hidden);
	}

	UE_LOG(LogTemp, Log, TEXT("[PlayerHUDComponent] In-game setting widget ready. class=%s widget=%s"), *GetNameSafe(InGameSettingClass.Get()), *GetNameSafe(InGameSettingWidget));

	auto FindWidgetByNames = [&](std::initializer_list<const TCHAR*> Names) -> UWidget*
	{
		for (const TCHAR* WidgetName : Names)
		{
			if (UWidget* FoundWidget = InGameSettingWidget->GetWidgetFromName(FName(WidgetName)))
			{
				return FoundWidget;
			}
		}
		return nullptr;
	};

	ResolutionComboBox = Cast<UComboBoxString>(FindWidgetByNames({ TEXT("ResolutionComboBox"), TEXT("VideoResolutionComboBox") }));
	FullscreenModeComboBox = Cast<UComboBoxString>(FindWidgetByNames({ TEXT("FullscreenModeComboBox"), TEXT("ScreenModeComboBox") }));
	FrameLimitComboBox = Cast<UComboBoxString>(FindWidgetByNames({ TEXT("FrameLimitComboBox"), TEXT("FrameRateComboBox"), TEXT("FPSComboBox") }));
	VSyncCheckBox = Cast<UCheckBox>(FindWidgetByNames({ TEXT("VSyncCheckBox"), TEXT("VerticalSyncCheckBox") }));
	MouseSensitivitySlider = Cast<USlider>(FindWidgetByNames({ TEXT("Mouse_Slider"), TEXT("MouseSensitivitySlider") }));
	MasterVolumeSlider = Cast<USlider>(FindWidgetByNames({ TEXT("MasterVolume_Slider") }));
	BGMVolumeSlider = Cast<USlider>(FindWidgetByNames({ TEXT("BGM_Slider") }));
	SFXVolumeSlider = Cast<USlider>(FindWidgetByNames({ TEXT("SFX_Slider") }));
	InterfaceVolumeSlider = Cast<USlider>(FindWidgetByNames({ TEXT("Interface_Slider") }));
	ResolutionApplyBtn = Cast<UButton>(FindWidgetByNames({ TEXT("ResolutionApplyBtn"), TEXT("ApplyBtn") }));
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

	if (MouseSensitivitySlider)
	{
		MouseSensitivitySlider->OnValueChanged.RemoveDynamic(this, &UPlayerHUDComponent::OnMouseSensitivitySliderChanged);
		MouseSensitivitySlider->OnValueChanged.AddDynamic(this, &UPlayerHUDComponent::OnMouseSensitivitySliderChanged);
	}

	EnsureVideoSettingOptions();
	SyncVideoSettingsToCurrent();
	SyncMouseSensitivitySliderToCurrent();
	SyncAudioSlidersToCurrent();
	ApplyAudioSettings();
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

	SyncVideoSettingsToCurrent();
	SyncMouseSensitivitySliderToCurrent();
	SyncAudioSlidersToCurrent();
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

void UPlayerHUDComponent::EnsureVideoSettingOptions()
{
	if (ResolutionComboBox && ResolutionComboBox->GetOptionCount() == 0)
	{
		EnsureComboHasOption(ResolutionComboBox, TEXT("1280x720"));
		EnsureComboHasOption(ResolutionComboBox, TEXT("1600x900"));
		EnsureComboHasOption(ResolutionComboBox, TEXT("1920x1080"));
		EnsureComboHasOption(ResolutionComboBox, TEXT("2560x1440"));
	}

	if (FullscreenModeComboBox && FullscreenModeComboBox->GetOptionCount() == 0)
	{
		EnsureComboHasOption(FullscreenModeComboBox, TEXT("전체화면"));
		EnsureComboHasOption(FullscreenModeComboBox, TEXT("전체화면 창"));
		EnsureComboHasOption(FullscreenModeComboBox, TEXT("창모드"));
	}

	if (FrameLimitComboBox && FrameLimitComboBox->GetOptionCount() == 0)
	{
		EnsureComboHasOption(FrameLimitComboBox, TEXT("30"));
		EnsureComboHasOption(FrameLimitComboBox, TEXT("60"));
		EnsureComboHasOption(FrameLimitComboBox, TEXT("120"));
		EnsureComboHasOption(FrameLimitComboBox, TEXT("144"));
		EnsureComboHasOption(FrameLimitComboBox, TEXT("240"));
		EnsureComboHasOption(FrameLimitComboBox, TEXT("무제한"));
	}
}

void UPlayerHUDComponent::SyncVideoSettingsToCurrent()
{
	SyncResolutionComboToCurrent();
	SyncFullscreenModeComboToCurrent();
	SyncFrameLimitComboToCurrent();
	SyncVSyncCheckBoxToCurrent();
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

void UPlayerHUDComponent::SyncFullscreenModeComboToCurrent()
{
	if (!FullscreenModeComboBox)
	{
		return;
	}

	if (UGameUserSettings* GameUserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		switch (GameUserSettings->GetFullscreenMode())
		{
		case EWindowMode::Fullscreen:
			FullscreenModeComboBox->SetSelectedOption(TEXT("전체화면"));
			return;
		case EWindowMode::WindowedFullscreen:
			FullscreenModeComboBox->SetSelectedOption(TEXT("전체화면 창"));
			return;
		case EWindowMode::Windowed:
		default:
			FullscreenModeComboBox->SetSelectedOption(TEXT("창모드"));
			return;
		}
	}
}

void UPlayerHUDComponent::SyncFrameLimitComboToCurrent()
{
	if (!FrameLimitComboBox)
	{
		return;
	}

	if (UGameUserSettings* GameUserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		const float FrameLimit = GameUserSettings->GetFrameRateLimit();
		if (FrameLimit <= 0.0f)
		{
			FrameLimitComboBox->SetSelectedOption(TEXT("무제한"));
			return;
		}

		FrameLimitComboBox->SetSelectedOption(FString::FromInt(FMath::RoundToInt(FrameLimit)));
	}
}

void UPlayerHUDComponent::SyncVSyncCheckBoxToCurrent()
{
	if (!VSyncCheckBox)
	{
		return;
	}

	if (UGameUserSettings* GameUserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		VSyncCheckBox->SetIsChecked(GameUserSettings->IsVSyncEnabled());
	}
}

bool UPlayerHUDComponent::TryParseFullscreenModeString(const FString& InOption, EWindowMode::Type& OutWindowMode) const
{
	FString Option = InOption;
	Option.TrimStartAndEndInline();
	if (Option.IsEmpty())
	{
		return false;
	}

	if (Option.Contains(TEXT("창모드")) || Option.Equals(TEXT("Windowed"), ESearchCase::IgnoreCase))
	{
		OutWindowMode = EWindowMode::Windowed;
		return true;
	}

	if (Option.Contains(TEXT("전체화면 창")) || Option.Contains(TEXT("테두리")) || Option.Equals(TEXT("Borderless"), ESearchCase::IgnoreCase) || Option.Equals(TEXT("WindowedFullscreen"), ESearchCase::IgnoreCase))
	{
		OutWindowMode = EWindowMode::WindowedFullscreen;
		return true;
	}

	if (Option.Contains(TEXT("전체화면")) || Option.Equals(TEXT("Fullscreen"), ESearchCase::IgnoreCase))
	{
		OutWindowMode = EWindowMode::Fullscreen;
		return true;
	}

	return false;
}

bool UPlayerHUDComponent::TryParseFrameLimitString(const FString& InOption, float& OutFrameLimit) const
{
	FString Option = InOption;
	Option.TrimStartAndEndInline();
	if (Option.IsEmpty())
	{
		return false;
	}

	if (Option.Contains(TEXT("무제한")) || Option.Equals(TEXT("Unlimited"), ESearchCase::IgnoreCase) || Option.Equals(TEXT("Off"), ESearchCase::IgnoreCase))
	{
		OutFrameLimit = 0.0f;
		return true;
	}

	Option.ReplaceInline(TEXT("FPS"), TEXT(""), ESearchCase::IgnoreCase);
	Option.ReplaceInline(TEXT("fps"), TEXT(""), ESearchCase::IgnoreCase);
	Option.TrimStartAndEndInline();

	const float ParsedValue = FCString::Atof(*Option);
	if (ParsedValue <= 0.0f)
	{
		return false;
	}

	OutFrameLimit = ParsedValue;
	return true;
}

void UPlayerHUDComponent::LoadMouseSensitivitySetting()
{
	float LoadedValue = DefaultMouseSensitivityMultiplier;
	if (GConfig)
	{
		GConfig->GetFloat(LocalSettingsSection, MouseSensitivityConfigKey, LoadedValue, GGameUserSettingsIni);
	}

	MouseSensitivityMultiplier = FMath::Clamp(LoadedValue, MinMouseSensitivityMultiplier, MaxMouseSensitivityMultiplier);
}

void UPlayerHUDComponent::SaveMouseSensitivitySetting(float NewValue)
{
	MouseSensitivityMultiplier = FMath::Clamp(NewValue, MinMouseSensitivityMultiplier, MaxMouseSensitivityMultiplier);
	if (GConfig)
	{
		GConfig->SetFloat(LocalSettingsSection, MouseSensitivityConfigKey, MouseSensitivityMultiplier, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

void UPlayerHUDComponent::SyncMouseSensitivitySliderToCurrent()
{
	if (MouseSensitivitySlider)
	{
		MouseSensitivitySlider->SetValue(MouseSensitivityMultiplier);
	}
}

void UPlayerHUDComponent::OnMouseSensitivitySliderChanged(float NewValue)
{
	MouseSensitivityMultiplier = FMath::Clamp(NewValue, MinMouseSensitivityMultiplier, MaxMouseSensitivityMultiplier);
}

void UPlayerHUDComponent::LoadAudioSettings()
{
	MasterVolumeValue = 1.0f;
	BGMVolumeValue = 1.0f;
	SFXVolumeValue = 1.0f;
	InterfaceVolumeValue = 1.0f;

	if (GConfig)
	{
		GConfig->GetFloat(LocalSettingsSection, MasterVolumeConfigKey, MasterVolumeValue, GGameUserSettingsIni);
		GConfig->GetFloat(LocalSettingsSection, BGMVolumeConfigKey, BGMVolumeValue, GGameUserSettingsIni);
		GConfig->GetFloat(LocalSettingsSection, SFXVolumeConfigKey, SFXVolumeValue, GGameUserSettingsIni);
		GConfig->GetFloat(LocalSettingsSection, InterfaceVolumeConfigKey, InterfaceVolumeValue, GGameUserSettingsIni);
	}

	MasterVolumeValue = FMath::Clamp(MasterVolumeValue, 0.0f, 1.0f);
	BGMVolumeValue = FMath::Clamp(BGMVolumeValue, 0.0f, 1.0f);
	SFXVolumeValue = FMath::Clamp(SFXVolumeValue, 0.0f, 1.0f);
	InterfaceVolumeValue = FMath::Clamp(InterfaceVolumeValue, 0.0f, 1.0f);
}

void UPlayerHUDComponent::SaveAudioSettings()
{
	MasterVolumeValue = FMath::Clamp(MasterVolumeValue, 0.0f, 1.0f);
	BGMVolumeValue = FMath::Clamp(BGMVolumeValue, 0.0f, 1.0f);
	SFXVolumeValue = FMath::Clamp(SFXVolumeValue, 0.0f, 1.0f);
	InterfaceVolumeValue = FMath::Clamp(InterfaceVolumeValue, 0.0f, 1.0f);

	if (GConfig)
	{
		GConfig->SetFloat(LocalSettingsSection, MasterVolumeConfigKey, MasterVolumeValue, GGameUserSettingsIni);
		GConfig->SetFloat(LocalSettingsSection, BGMVolumeConfigKey, BGMVolumeValue, GGameUserSettingsIni);
		GConfig->SetFloat(LocalSettingsSection, SFXVolumeConfigKey, SFXVolumeValue, GGameUserSettingsIni);
		GConfig->SetFloat(LocalSettingsSection, InterfaceVolumeConfigKey, InterfaceVolumeValue, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

void UPlayerHUDComponent::SyncAudioSlidersToCurrent()
{
	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->SetValue(MasterVolumeValue);
	}
	if (BGMVolumeSlider)
	{
		BGMVolumeSlider->SetValue(BGMVolumeValue);
	}
	if (SFXVolumeSlider)
	{
		SFXVolumeSlider->SetValue(SFXVolumeValue);
	}
	if (InterfaceVolumeSlider)
	{
		InterfaceVolumeSlider->SetValue(InterfaceVolumeValue);
	}
}

void UPlayerHUDComponent::ApplyAudioSettings()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!RuntimeAudioSettingsMix)
	{
		RuntimeAudioSettingsMix = NewObject<USoundMix>(this, TEXT("RuntimeAudioSettingsMix"));
	}
	if (!RuntimeAudioSettingsMix)
	{
		return;
	}

	auto ApplyClassVolume = [&](USoundClass* SoundClass, float Volume)
	{
		if (!SoundClass)
		{
			return;
		}

		UGameplayStatics::SetSoundMixClassOverride(
			World,
			RuntimeAudioSettingsMix,
			SoundClass,
			FMath::Clamp(Volume, 0.0f, 1.0f),
			1.0f,
			0.05f,
			true
		);
	};

	ApplyClassVolume(MasterSoundClass, MasterVolumeValue);
	ApplyClassVolume(BGMSoundClass, BGMVolumeValue);
	ApplyClassVolume(SFXSoundClass, SFXVolumeValue);
	ApplyClassVolume(InterfaceSoundClass, InterfaceVolumeValue);
	UGameplayStatics::PushSoundMixModifier(World, RuntimeAudioSettingsMix);
}

void UPlayerHUDComponent::OnResolutionApplyClicked()
{
	AMyPlayerController* OwnerController = GetOwnerController();
	if (!OwnerController || (!ResolutionComboBox && !FullscreenModeComboBox && !FrameLimitComboBox && !VSyncCheckBox))
	{
		return;
	}

	FIntPoint TargetResolution = FIntPoint::ZeroValue;
	EWindowMode::Type TargetWindowMode = EWindowMode::Windowed;
	float TargetFrameLimit = 0.0f;
	bool bHasResolutionSelection = false;
	bool bHasWindowModeSelection = false;
	bool bHasFrameLimitSelection = false;

	if (ResolutionComboBox)
	{
		const FString SelectedResolution = ResolutionComboBox->GetSelectedOption();
		if (!SelectedResolution.IsEmpty())
		{
			bHasResolutionSelection = TryParseResolutionString(SelectedResolution, TargetResolution);
		}
	}

	if (FullscreenModeComboBox)
	{
		const FString SelectedWindowMode = FullscreenModeComboBox->GetSelectedOption();
		if (!SelectedWindowMode.IsEmpty())
		{
			bHasWindowModeSelection = TryParseFullscreenModeString(SelectedWindowMode, TargetWindowMode);
		}
	}

	if (FrameLimitComboBox)
	{
		const FString SelectedFrameLimit = FrameLimitComboBox->GetSelectedOption();
		if (!SelectedFrameLimit.IsEmpty())
		{
			bHasFrameLimitSelection = TryParseFrameLimitString(SelectedFrameLimit, TargetFrameLimit);
		}
	}

	if (UGameUserSettings* GameUserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		if (bHasWindowModeSelection)
		{
			GameUserSettings->SetFullscreenMode(TargetWindowMode);
		}

		if (bHasResolutionSelection)
		{
			GameUserSettings->SetScreenResolution(TargetResolution);
		}

		if (bHasFrameLimitSelection)
		{
			GameUserSettings->SetFrameRateLimit(TargetFrameLimit);
		}

		if (VSyncCheckBox)
		{
			GameUserSettings->SetVSyncEnabled(VSyncCheckBox->IsChecked());
		}

		if (MouseSensitivitySlider)
		{
			SaveMouseSensitivitySetting(MouseSensitivitySlider->GetValue());
		}

		if (MasterVolumeSlider)
		{
			MasterVolumeValue = MasterVolumeSlider->GetValue();
		}
		if (BGMVolumeSlider)
		{
			BGMVolumeValue = BGMVolumeSlider->GetValue();
		}
		if (SFXVolumeSlider)
		{
			SFXVolumeValue = SFXVolumeSlider->GetValue();
		}
		if (InterfaceVolumeSlider)
		{
			InterfaceVolumeValue = InterfaceVolumeSlider->GetValue();
		}

		SaveAudioSettings();
		ApplyAudioSettings();

		GameUserSettings->ApplyResolutionSettings(false);
		GameUserSettings->ApplySettings(false);
		GameUserSettings->SaveSettings();

		if (bHasResolutionSelection)
		{
			const TCHAR WindowModeSuffix = (bHasWindowModeSelection && TargetWindowMode == EWindowMode::Windowed) ? TEXT('w') : TEXT('f');
			const FString SetResCommand = FString::Printf(TEXT("r.SetRes %dx%d%c"), TargetResolution.X, TargetResolution.Y, WindowModeSuffix);
			OwnerController->ConsoleCommand(SetResCommand, true);
		}

		SyncVideoSettingsToCurrent();
		SyncMouseSensitivitySliderToCurrent();
		SyncAudioSlidersToCurrent();
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

	if (UButton* CloseButton = FindInspectMaliceButton(TEXT("SelectUserCloseBtn")))
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UPlayerHUDComponent::OnInspectMaliceCloseClicked);
		CloseButton->OnClicked.AddDynamic(this, &UPlayerHUDComponent::OnInspectMaliceCloseClicked);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[InspectMalice] Missing close button widget: SelectUserCloseBtn"));
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

		if (!IsInspectMaliceCandidateInScope(LocalPlayerState, CandidatePlayerState))
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
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[InspectMalice] Missing button widget: %s"), *ButtonName.ToString());
		}

		const FName TextName(*FString::Printf(TEXT("UserText%d"), WidgetIndex));
		if (UTextBlock* TargetText = FindInspectMaliceText(TextName))
		{
			TargetText->SetText(FText::FromString(ResolvePlayerDisplayName(CandidatePlayerState)));
			TargetText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[InspectMalice] Missing text widget: %s"), *TextName.ToString());
		}
	}

	if (InspectMaliceSelectablePlayers.Num() == 0 && GameState->PlayerArray.Num() > 1)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[InspectMalice] No selectable players found. localPlayer=%s room=%s totalPlayers=%d"),
			*GetNameSafe(LocalPlayerState),
			*A302RoomScope::ResolvePlayerRoomCode(LocalPlayerState),
			GameState->PlayerArray.Num()
		);

		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				InspectMalicePopulateRetryHandle,
				this,
				&UPlayerHUDComponent::RetryPopulateInspectMaliceSelectionWidget,
				0.25f,
				false
			);
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

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InspectMaliceHideTimerHandle);
		World->GetTimerManager().ClearTimer(InspectMaliceSelectionTimeoutHandle);
		World->GetTimerManager().ClearTimer(InspectMalicePopulateRetryHandle);
		World->GetTimerManager().ClearTimer(InspectMaliceItemTimerTickHandle);
	}

	StopInspectMaliceItemTimer();
	bInspectMaliceSelectionConsumed = false;

	FInputModeGameOnly InputMode;
	OwnerController->SetInputMode(InputMode);
	OwnerController->bShowMouseCursor = false;
	OwnerController->bEnableClickEvents = false;
	OwnerController->bEnableMouseOverEvents = false;
}

void UPlayerHUDComponent::RetryPopulateInspectMaliceSelectionWidget()
{
	if (!InspectMaliceWidgetInstance || InspectMaliceWidgetInstance->GetVisibility() != ESlateVisibility::Visible || bInspectMaliceSelectionConsumed)
	{
		return;
	}

	ResetInspectMaliceSelectionWidget();
	PopulateInspectMaliceSelectionWidget();
	SetInspectMaliceSelectionButtonsEnabled(true);
}

void UPlayerHUDComponent::ApplyInspectMaliceSelection(int32 EntryIndex)
{
	if (bInspectMaliceSelectionConsumed)
	{
		return;
	}

	if (!InspectMaliceSelectablePlayers.IsValidIndex(EntryIndex))
	{
		return;
	}

	APlayerState* TargetPlayerState = InspectMaliceSelectablePlayers[EntryIndex];
	if (!TargetPlayerState)
	{
		return;
	}

	bInspectMaliceSelectionConsumed = true;
	SetInspectMaliceSelectionButtonsEnabled(false);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InspectMalicePopulateRetryHandle);
		World->GetTimerManager().ClearTimer(InspectMaliceSelectionTimeoutHandle);
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
	StartInspectMaliceItemTimer(InspectMaliceResultDisplaySeconds);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			InspectMaliceHideTimerHandle,
			this,
			&UPlayerHUDComponent::HideInspectMaliceSelectionWidget,
			InspectMaliceResultDisplaySeconds,
			false
		);
	}
}

void UPlayerHUDComponent::HandleInspectMaliceSelectionTimeout()
{
	if (bInspectMaliceSelectionConsumed)
	{
		return;
	}

	HideInspectMaliceSelectionWidget();
}

void UPlayerHUDComponent::SetInspectMaliceSelectionButtonsEnabled(bool bEnabled)
{
	for (int32 WidgetIndex = 1; WidgetIndex <= PlayerHUDVoteSlotCount; ++WidgetIndex)
	{
		const FName ButtonName(*FString::Printf(TEXT("UserBtn%d"), WidgetIndex));
		if (UButton* TargetButton = FindInspectMaliceButton(ButtonName))
		{
			if (TargetButton->GetVisibility() == ESlateVisibility::Visible)
			{
				TargetButton->SetIsEnabled(bEnabled);
			}
		}
	}
}

void UPlayerHUDComponent::StartInspectMaliceItemTimer(float DurationSeconds)
{
	UWorld* World = GetWorld();
	UTextBlock* ItemTimerText = FindItemTimerText();
	if (!World || !ItemTimerText)
	{
		return;
	}

	if (!bInspectMaliceItemTimerBaselineCaptured)
	{
		bInspectMaliceItemTimerWasVisible = ItemTimerText->GetVisibility() == ESlateVisibility::Visible;
		bInspectMaliceItemTimerBaselineCaptured = true;
	}

	InspectMaliceItemTimerEndTime = World->GetTimeSeconds() + FMath::Max(0.1f, DurationSeconds);
	SetItemTimerVisible(true);
	UpdateItemTimerText(DurationSeconds);

	World->GetTimerManager().ClearTimer(InspectMaliceItemTimerTickHandle);
	World->GetTimerManager().SetTimer(
		InspectMaliceItemTimerTickHandle,
		this,
		&UPlayerHUDComponent::TickInspectMaliceItemTimer,
		0.1f,
		true
	);
}

void UPlayerHUDComponent::StopInspectMaliceItemTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(InspectMaliceItemTimerTickHandle);
	}

	InspectMaliceItemTimerEndTime = 0.0f;
	if (bInspectMaliceItemTimerBaselineCaptured && !bInspectMaliceItemTimerWasVisible)
	{
		SetItemTimerVisible(false);
	}
	else if (bMatchTimerVisible)
	{
		TickMatchTimer();
	}

	bInspectMaliceItemTimerBaselineCaptured = false;
	bInspectMaliceItemTimerWasVisible = false;
}

void UPlayerHUDComponent::TickInspectMaliceItemTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float RemainingSeconds = FMath::Max(0.0f, InspectMaliceItemTimerEndTime - World->GetTimeSeconds());
	UpdateItemTimerText(RemainingSeconds);

	if (RemainingSeconds <= 0.0f)
	{
		World->GetTimerManager().ClearTimer(InspectMaliceItemTimerTickHandle);
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

void UPlayerHUDComponent::OnInspectMaliceCloseClicked()
{
	HideInspectMaliceSelectionWidget();
}
