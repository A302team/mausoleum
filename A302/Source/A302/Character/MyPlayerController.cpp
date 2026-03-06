#include "Character/MyPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
	constexpr int32 PlayerControllerQuickSlotCount = 5;
}

UUserWidget* AMyPlayerController::FindQuickSlotWidget(int32 SlotIndex) const
{
	if (!QuickSlotBarWidget || SlotIndex < 0 || SlotIndex >= PlayerControllerQuickSlotCount)
	{
		return nullptr;
	}

	const FName SlotWidgetName(*FString::Printf(TEXT("QuickSlot%d"), SlotIndex + 1));
	return Cast<UUserWidget>(QuickSlotBarWidget->GetWidgetFromName(SlotWidgetName));
}

UTextBlock* AMyPlayerController::FindQuickSlotItemNameText(int32 SlotIndex) const
{
	if (UUserWidget* QuickSlotWidget = FindQuickSlotWidget(SlotIndex))
	{
		return Cast<UTextBlock>(QuickSlotWidget->GetWidgetFromName(TEXT("ItemName")));
	}

	return nullptr;
}

UImage* AMyPlayerController::FindQuickSlotItemIconImage(int32 SlotIndex) const
{
	if (UUserWidget* QuickSlotWidget = FindQuickSlotWidget(SlotIndex))
	{
		if (UImage* KnifeIcon = Cast<UImage>(QuickSlotWidget->GetWidgetFromName(TEXT("KnifeIcon"))))
		{
			return KnifeIcon;
		}

		return Cast<UImage>(QuickSlotWidget->GetWidgetFromName(TEXT("ItemIcon")));
	}

	return nullptr;
}

UImage* AMyPlayerController::FindQuickSlotItemSelectedImage(int32 SlotIndex) const
{
	if (UUserWidget* QuickSlotWidget = FindQuickSlotWidget(SlotIndex))
	{
		return Cast<UImage>(QuickSlotWidget->GetWidgetFromName(TEXT("ItemSelected")));
	}

	return nullptr;
}

UTextBlock* AMyPlayerController::FindShieldCountText() const
{
	if (!QuickSlotBarWidget)
	{
		return nullptr;
	}

	return Cast<UTextBlock>(QuickSlotBarWidget->GetWidgetFromName(TEXT("ShieldCount")));
}

UTextBlock* AMyPlayerController::FindMaliceCountText() const
{
	if (!QuickSlotBarWidget)
	{
		return nullptr;
	}

	return Cast<UTextBlock>(QuickSlotBarWidget->GetWidgetFromName(TEXT("MaliceCount")));
}

UTextBlock* AMyPlayerController::FindItemTimerText() const
{
	if (!QuickSlotBarWidget)
	{
		return nullptr;
	}

	return Cast<UTextBlock>(QuickSlotBarWidget->GetWidgetFromName(TEXT("ItemTimer")));
}

bool AMyPlayerController::UpdateQuickSlotItemVisual(int32 SlotIndex, const FText& ItemName, UTexture2D* ItemIcon)
{
	bool bNameUpdated = false;
	bool bIconWidgetFound = false;
	bool bIconTextureSet = false;

	if (UTextBlock* ItemNameText = FindQuickSlotItemNameText(SlotIndex))
	{
		ItemNameText->SetText(ItemName);
		ItemNameText->SetVisibility(ESlateVisibility::Visible);
		bNameUpdated = true;
	}

	if (UImage* ItemIconImage = FindQuickSlotItemIconImage(SlotIndex))
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

bool AMyPlayerController::UpdateQuickSlotItemName(int32 SlotIndex, const FText& ItemName)
{
	return UpdateQuickSlotItemVisual(SlotIndex, ItemName, nullptr);
}

void AMyPlayerController::UpdateQuickSlotSelectionVisual(int32 SelectedSlotIndex)
{
	for (int32 SlotIndex = 0; SlotIndex < PlayerControllerQuickSlotCount; ++SlotIndex)
	{
		if (UImage* SelectedImage = FindQuickSlotItemSelectedImage(SlotIndex))
		{
			SelectedImage->SetVisibility(
				SlotIndex == SelectedSlotIndex ? ESlateVisibility::Visible : ESlateVisibility::Hidden
			);
		}
	}
}

bool AMyPlayerController::UpdateShieldCountText(int32 ShieldCount)
{
	UTextBlock* ShieldCountText = FindShieldCountText();
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
	UTextBlock* MaliceCountText = FindMaliceCountText();
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
	UTextBlock* ItemTimerText = FindItemTimerText();
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
	if (UTextBlock* ItemTimerText = FindItemTimerText())
	{
		ItemTimerText->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}

bool AMyPlayerController::IsInGameSettingMenuOpen() const
{
	return InGameSettingWidget && InGameSettingWidget->GetVisibility() == ESlateVisibility::Visible;
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
}

void AMyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (DefaultMappingContext)
			{
				Subsys->AddMappingContext(DefaultMappingContext, MappingPriority);
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[PC] DefaultMappingContext is NULL"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[PC] EnhancedInputLocalPlayerSubsystem NULL"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[PC] LocalPlayer NULL"));
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
}

void AMyPlayerController::InitializeQuickSlotVisualState()
{
	for (int32 SlotIndex = 0; SlotIndex < PlayerControllerQuickSlotCount; ++SlotIndex)
	{
		if (UTextBlock* ItemNameText = FindQuickSlotItemNameText(SlotIndex))
		{
			ItemNameText->SetText(FText::GetEmpty());
			ItemNameText->SetVisibility(ESlateVisibility::Hidden);
		}

		if (UImage* ItemIconImage = FindQuickSlotItemIconImage(SlotIndex))
		{
			ItemIconImage->SetBrushFromTexture(nullptr, true);
			ItemIconImage->SetVisibility(ESlateVisibility::Hidden);
		}

		if (UImage* SelectedImage = FindQuickSlotItemSelectedImage(SlotIndex))
		{
			SelectedImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}
