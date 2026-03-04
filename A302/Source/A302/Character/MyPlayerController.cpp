#include "Character/MyPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

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
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[PC] QuickSlotBarClass is NULL"));
	}
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
	}
}
