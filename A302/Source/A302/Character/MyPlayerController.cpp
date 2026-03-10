#include "Character/MyPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Character/MyCharacter.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "EnhancedInputSubsystems.h"
#include "GamePlay/Events/BaseEvent.h"
#include "InputMappingContext.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameMode/A302GameMode.h"
#include "UI/ChatWidget.h"
#include "UI/PersonalEventWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

namespace
{
	constexpr int32 PlayerControllerQuickSlotCount = 5;
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