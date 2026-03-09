#include "Character/MyPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "GameMode/A302GameMode.h"
#include "UI/ChatWidget.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr int32 PlayerControllerQuickSlotCount = 5;
}

bool AMyPlayerController::ServerRequestGameStart_Validate()
{
    return true;
}

void AMyPlayerController::ServerRequestGameStart_Implementation()
{
    UWorld* World = GetWorld();
    if (!World) return;
    
    UE_LOG(LogTemp, Log, TEXT("[PC] ServerRequestGameStart - NetMode: %d"), (int32)World->GetNetMode());
    World->ServerTravel(TEXT("/Game/PersonalWorkSpace/sikk806/MyTestLevel?listen"));
}

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

AMyPlayerController::AMyPlayerController()
{
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
}