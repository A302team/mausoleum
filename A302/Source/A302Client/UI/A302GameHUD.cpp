#include "UI/A302GameHUD.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "UI/EscapeWaitingWidget.h"
#include "UI/PersonalEventWidget.h"
#include "UI/PlayerHUDComponent.h"
#include "UI/StatueProgressWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Blueprint/WidgetTree.h"

AA302GameHUD::AA302GameHUD()
{
	static ConstructorHelpers::FClassFinder<UUserWidget> QuickSlotBarBPClass(TEXT("/Game/WorkSpace/UI/WBP_HUD2.WBP_HUD2_C"));
	if (QuickSlotBarBPClass.Succeeded())
	{
		QuickSlotBarClass = QuickSlotBarBPClass.Class;
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> InGameSettingBPClass(TEXT("/Game/WorkSpace/UI/WBP_InGameSetting.WBP_InGameSetting_C"));
	if (InGameSettingBPClass.Succeeded())
	{
		InGameSettingClass = InGameSettingBPClass.Class;
	}
	else
	{
		static ConstructorHelpers::FClassFinder<UUserWidget> InGameSettingClassLegacy(TEXT("/Game/WorkSpace/UI/WBP_InGameSetting2.WBP_InGameSetting2_C"));
		if (InGameSettingClassLegacy.Succeeded())
		{
			InGameSettingClass = InGameSettingClassLegacy.Class;
		}
	}

	if (UClass* LoadedPersonalEventClass = LoadClass<UPersonalEventWidget>(
		nullptr,
		TEXT("/Game/WorkSpace/UI/WBP_PersonalEvent.WBP_PersonalEvent_C"),
		nullptr,
		LOAD_NoWarn))
	{
		PersonalEventWidgetClass = LoadedPersonalEventClass;
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> InspectMaliceWidgetBPClass(TEXT("/Game/WorkSpace/UI/PersonalEvent/WBP_SelectUser"));
	if (InspectMaliceWidgetBPClass.Succeeded())
	{
		InspectMaliceWidgetClass = InspectMaliceWidgetBPClass.Class;
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> GroupEventVoteWidgetBPClass(TEXT("/Game/WorkSpace/UI/GroupEvent/WBP_GroupEventVote"));
	if (GroupEventVoteWidgetBPClass.Succeeded())
	{
		GroupEventVoteWidgetClass = GroupEventVoteWidgetBPClass.Class;
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> TitleCardWidgetBPClass(TEXT("/Game/WorkSpace/UI/WBP_TitleCard"));
	if (TitleCardWidgetBPClass.Succeeded())
	{
		TitleCardWidgetClass = TitleCardWidgetBPClass.Class;
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> PhaseTransitionWidgetBPClass(TEXT("/Game/WorkSpace/UI/WBP_PhaseTransition"));
	if (PhaseTransitionWidgetBPClass.Succeeded())
	{
		PhaseTransitionWidgetClass = PhaseTransitionWidgetBPClass.Class;
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> ItemDescriptionWidgetBPClass(TEXT("/Game/WorkSpace/UI/WBP_ItemDescription"));
	if (ItemDescriptionWidgetBPClass.Succeeded())
	{
		ItemDescriptionWidgetClass = ItemDescriptionWidgetBPClass.Class;
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> ResultWidgetBPClass(TEXT("/Game/WorkSpace/UI/WBP_Result"));
	if (ResultWidgetBPClass.Succeeded())
	{
		ResultWidgetClass = ResultWidgetBPClass.Class;
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> DieWidgetBPClass(TEXT("/Game/WorkSpace/UI/WBP_Die"));
	if (DieWidgetBPClass.Succeeded())
	{
		DieWidgetClass = DieWidgetBPClass.Class;
	}

	static ConstructorHelpers::FClassFinder<UUserWidget> NotificationListWidgetBPClass(TEXT("/Game/WorkSpace/UI/WBP_NotificationList"));
	if (NotificationListWidgetBPClass.Succeeded())
	{
		NotificationListWidgetClass = NotificationListWidgetBPClass.Class;
	}

	PlayerHUDComponent = CreateDefaultSubobject<UPlayerHUDComponent>(TEXT("PlayerHUDComponent"));
}

void AA302GameHUD::BeginPlay()
{
	Super::BeginPlay();
}

void AA302GameHUD::InitializeClientInGameWidgets()
{
	if (!PlayerHUDComponent)
	{
		PlayerHUDComponent = Cast<UPlayerHUDComponent>(GetComponentByClass(UPlayerHUDComponent::StaticClass()));
	}

	if (!PlayerHUDComponent)
	{
		PlayerHUDComponent = NewObject<UPlayerHUDComponent>(this, TEXT("PlayerHUDComponent"));
		if (PlayerHUDComponent)
		{
			PlayerHUDComponent->RegisterComponent();
		}
	}

	if (!QuickSlotBarClass)
	{
		if (UClass* LoadedQuickSlotClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WorkSpace/UI/WBP_HUD2.WBP_HUD2_C")))
		{
			QuickSlotBarClass = LoadedQuickSlotClass;
		}
	}

	if (!InGameSettingClass)
	{
		if (UClass* LoadedInGameSettingClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WorkSpace/UI/WBP_InGameSetting.WBP_InGameSetting_C")))
		{
			InGameSettingClass = LoadedInGameSettingClass;
		}
	}

	if (!PersonalEventWidgetClass)
	{
		if (UClass* LoadedPersonalEventClass = LoadClass<UPersonalEventWidget>(nullptr, TEXT("/Game/WorkSpace/UI/WBP_PersonalEvent.WBP_PersonalEvent_C")))
		{
			PersonalEventWidgetClass = LoadedPersonalEventClass;
		}
	}

	if (StatueProgressWidgetClass && !StatueProgressWidgetInstance)
	{
		APlayerController* PC = GetOwningPlayerController();
		if (PC)
		{
			StatueProgressWidgetInstance = CreateWidget<UStatueProgressWidget>(PC, StatueProgressWidgetClass);
			if (StatueProgressWidgetInstance)
			{
				StatueProgressWidgetInstance->AddToViewport(15);
				// Visibility는 내부 컴포넌트(StatueProgressWidget)가 알아서 처리함
			}
		}
	}

	GetOrCreateNotificationListWidget();

	InitializeChatWidget();

	if (PlayerHUDComponent)
	{
		PlayerHUDComponent->InitializeInGameHUD(QuickSlotBarClass, InGameSettingClass, InspectMaliceWidgetClass);
		PlayerHUDComponent->RefreshQuickSlotBinding();
	}
}

UUserWidget* AA302GameHUD::GetOrCreateNotificationListWidget()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return nullptr;
	}

	if (!NotificationListWidgetClass)
	{
		if (UClass* LoadedNotificationListClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WorkSpace/UI/WBP_NotificationList.WBP_NotificationList_C")))
		{
			NotificationListWidgetClass = LoadedNotificationListClass;
		}
	}

	if (!NotificationListWidgetClass)
	{
		return nullptr;
	}

	if (!NotificationListWidgetInstance)
	{
		NotificationListWidgetInstance = CreateWidget<UUserWidget>(PC, NotificationListWidgetClass);
		if (NotificationListWidgetInstance)
		{
			NotificationListWidgetInstance->AddToViewport(220);
		}
	}
	else if (!NotificationListWidgetInstance->IsInViewport())
	{
		NotificationListWidgetInstance->AddToViewport(220);
	}

	return NotificationListWidgetInstance;
}

void AA302GameHUD::RefreshQuickSlotBinding()
{
	if (PlayerHUDComponent)
	{
		PlayerHUDComponent->RefreshQuickSlotBinding();
	}
}

void AA302GameHUD::InitializeChatWidget()
{
	if (UWorld* World = GetWorld())
	{
		FString MapName = World->GetMapName();
		// 로비(Lobby) 맵이 아니면 게임 중이므로 채팅 위젯을 생성하지 않음
		if (!MapName.Contains(TEXT("Lobby"), ESearchCase::IgnoreCase))
		{
			return; // 게임 레벨에서는 채팅 꺼짐
		}
	}

	if (ChatWidgetInstance)
	{
		return;
	}

	if (!ChatWidgetClass)
	{
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	ChatWidgetInstance = CreateWidget<UUserWidget>(PC, ChatWidgetClass);
	if (!ChatWidgetInstance)
	{
		return;
	}

	ChatWidgetInstance->AddToViewport();
}

void AA302GameHUD::ShowTitleCard(const FText& Title, const FText& Context, float DisplaySeconds)
{
	if (!TitleCardWidgetClass)
	{
		if (UClass* LoadedTitleCardClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WorkSpace/UI/WBP_TitleCard.WBP_TitleCard_C")))
		{
			TitleCardWidgetClass = LoadedTitleCardClass;
		}
	}

	if (!TitleCardWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[A302GameHUD] TitleCardWidgetClass is missing."));
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;

	if (!TitleCardWidgetInstance)
	{
		TitleCardWidgetInstance = CreateWidget<UUserWidget>(PC, TitleCardWidgetClass);
	}
	if (!TitleCardWidgetInstance)
	{
		return;
	}

	auto SetFirstAvailableText = [&](const TArray<FName>& CandidateNames, const FText& InText)
	{
		for (const FName& CandidateName : CandidateNames)
		{
			if (UTextBlock* TextBlock = Cast<UTextBlock>(TitleCardWidgetInstance->GetWidgetFromName(CandidateName)))
			{
				TextBlock->SetText(InText);
				return;
			}
		}
	};

	SetFirstAvailableText({ TEXT("EventTitle"), TEXT("TitleText"), TEXT("Txt_Title"), TEXT("ResultTitle") }, Title);
	SetFirstAvailableText({ TEXT("EventContext"), TEXT("DescriptionText"), TEXT("Txt_Description"), TEXT("ResultDescription"), TEXT("BodyText") }, Context);

	if (!TitleCardWidgetInstance->IsInViewport())
	{
		TitleCardWidgetInstance->AddToViewport(140);
	}

	TitleCardWidgetInstance->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TitleCardHideTimerHandle);
		if (DisplaySeconds > 0.0f)
		{
			World->GetTimerManager().SetTimer(
				TitleCardHideTimerHandle,
				this,
				&AA302GameHUD::HideTitleCard,
				FMath::Max(0.5f, DisplaySeconds),
				false
			);
		}
	}
}

void AA302GameHUD::HideTitleCard()
{
	if (TitleCardWidgetInstance)
	{
		TitleCardWidgetInstance->RemoveFromParent();
		TitleCardWidgetInstance = nullptr;
	}
}

void AA302GameHUD::ShowItemDescription(const FText& ItemName, const FText& ItemDescription, float DisplaySeconds)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	if (!ItemDescriptionWidgetClass)
	{
		if (UClass* LoadedItemDescriptionClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WorkSpace/UI/WBP_ItemDescription.WBP_ItemDescription_C")))
		{
			ItemDescriptionWidgetClass = LoadedItemDescriptionClass;
		}
	}

	if (!ItemDescriptionWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[A302GameHUD] Item description widget class is missing."));
		return;
	}

	if (!ItemDescriptionWidgetInstance)
	{
		ItemDescriptionWidgetInstance = CreateWidget<UUserWidget>(PC, ItemDescriptionWidgetClass);
	}

	if (!ItemDescriptionWidgetInstance)
	{
		return;
	}

	auto SetFirstAvailableText = [&](const TArray<FName>& CandidateNames, const FText& InText)
	{
		for (const FName& CandidateName : CandidateNames)
		{
			if (UTextBlock* TextBlock = Cast<UTextBlock>(ItemDescriptionWidgetInstance->GetWidgetFromName(CandidateName)))
			{
				TextBlock->SetText(InText);
				return;
			}
		}
	};

	SetFirstAvailableText({ TEXT("ItemName"), TEXT("TitleText"), TEXT("Txt_Title") }, ItemName);
	SetFirstAvailableText({ TEXT("ItemDescription"), TEXT("DescriptionText"), TEXT("Txt_Description"), TEXT("BodyText") }, ItemDescription);

	if (!ItemDescriptionWidgetInstance->IsInViewport())
	{
		ItemDescriptionWidgetInstance->AddToViewport(145);
	}

	ItemDescriptionWidgetInstance->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ItemDescriptionHideTimerHandle);
		if (DisplaySeconds > 0.0f)
		{
			World->GetTimerManager().SetTimer(
				ItemDescriptionHideTimerHandle,
				this,
				&AA302GameHUD::HideItemDescription,
				FMath::Max(0.5f, DisplaySeconds),
				false
			);
		}
	}
}

void AA302GameHUD::HideItemDescription()
{
	if (ItemDescriptionWidgetInstance)
	{
		ItemDescriptionWidgetInstance->RemoveFromParent();
		ItemDescriptionWidgetInstance = nullptr;
	}
}

void AA302GameHUD::StartPhaseTransition(const FText& Title, const FText& Context, float FadeOutSeconds, float HoldSeconds, float FadeInSeconds, float TitleDisplaySeconds)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	if (!PhaseTransitionWidgetClass)
	{
		if (UClass* LoadedPhaseTransitionClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WorkSpace/UI/WBP_PhaseTransition.WBP_PhaseTransition_C")))
		{
			PhaseTransitionWidgetClass = LoadedPhaseTransitionClass;
		}
	}

	if (!PhaseTransitionWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[A302GameHUD] Phase transition widget class is missing."));
		return;
	}

	if (!PhaseTransitionWidgetInstance)
	{
		PhaseTransitionWidgetInstance = CreateWidget<UUserWidget>(PC, PhaseTransitionWidgetClass);
	}

	if (!PhaseTransitionWidgetInstance)
	{
		return;
	}

	if (!PhaseTransitionWidgetInstance->IsInViewport())
	{
		PhaseTransitionWidgetInstance->AddToViewport(130);
	}

	PendingPhaseTransitionTitle = Title;
	PendingPhaseTransitionContext = Context;
	PendingPhaseHoldSeconds = FMath::Max(0.0f, HoldSeconds);
	PendingPhaseFadeInSeconds = FMath::Max(0.0f, FadeInSeconds);
	PendingPhaseTitleDisplaySeconds = FMath::Max(0.0f, TitleDisplaySeconds);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PhaseTransitionTickTimerHandle);
		World->GetTimerManager().ClearTimer(PhaseTransitionInputRestoreTimerHandle);
	}

	PhaseTransitionWidgetInstance->SetVisibility(ESlateVisibility::HitTestInvisible);
	SetPhaseTransitionOverlayOpacity(0.0f);
	SetPhaseTransitionInputLocked(true);
	BeginPhaseTransitionStep(EA302PhaseTransitionState::FadeToBlack, FadeOutSeconds);
}

void AA302GameHUD::TickPhaseTransition()
{
	UWorld* World = GetWorld();
	if (!World || !PhaseTransitionWidgetInstance)
	{
		return;
	}

	const float ElapsedSeconds = FMath::Max(World->GetTimeSeconds() - PhaseTransitionStepStartTime, 0.0f);
	const float Alpha = PhaseTransitionStepDuration <= KINDA_SMALL_NUMBER
		? 1.0f
		: FMath::Clamp(ElapsedSeconds / PhaseTransitionStepDuration, 0.0f, 1.0f);

	switch (PhaseTransitionState)
	{
	case EA302PhaseTransitionState::FadeToBlack:
		SetPhaseTransitionOverlayOpacity(Alpha);
		if (Alpha >= 1.0f)
		{
			BeginPhaseTransitionStep(EA302PhaseTransitionState::HoldBlack, PendingPhaseHoldSeconds);
		}
		break;
	case EA302PhaseTransitionState::HoldBlack:
		SetPhaseTransitionOverlayOpacity(1.0f);
		if (ElapsedSeconds >= PhaseTransitionStepDuration)
		{
			BeginPhaseTransitionStep(EA302PhaseTransitionState::FadeFromBlack, PendingPhaseFadeInSeconds);
		}
		break;
	case EA302PhaseTransitionState::FadeFromBlack:
		SetPhaseTransitionOverlayOpacity(1.0f - Alpha);
		if (Alpha >= 1.0f)
		{
			FinishPhaseTransition();
		}
		break;
	default:
		break;
	}
}

void AA302GameHUD::BeginPhaseTransitionStep(EA302PhaseTransitionState NewState, float DurationSeconds)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	PhaseTransitionState = NewState;
	PhaseTransitionStepDuration = FMath::Max(0.0f, DurationSeconds);
	PhaseTransitionStepStartTime = World->GetTimeSeconds();

	if (!World->GetTimerManager().IsTimerActive(PhaseTransitionTickTimerHandle))
	{
		World->GetTimerManager().SetTimer(
			PhaseTransitionTickTimerHandle,
			this,
			&AA302GameHUD::TickPhaseTransition,
			1.0f / 60.0f,
			true
		);
	}

	if (PhaseTransitionStepDuration <= KINDA_SMALL_NUMBER)
	{
		TickPhaseTransition();
	}
}

void AA302GameHUD::FinishPhaseTransition()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PhaseTransitionTickTimerHandle);
	}

	PhaseTransitionState = EA302PhaseTransitionState::Idle;
	SetPhaseTransitionOverlayOpacity(0.0f);

	if (PhaseTransitionWidgetInstance)
	{
		PhaseTransitionWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (!PendingPhaseTransitionTitle.IsEmpty() || !PendingPhaseTransitionContext.IsEmpty())
	{
		ShowTitleCard(PendingPhaseTransitionTitle, PendingPhaseTransitionContext, PendingPhaseTitleDisplaySeconds);
	}

	if (UWorld* World = GetWorld())
	{
		if (PendingPhaseTitleDisplaySeconds > 0.0f)
		{
			World->GetTimerManager().SetTimer(
				PhaseTransitionInputRestoreTimerHandle,
				FTimerDelegate::CreateUObject(this, &AA302GameHUD::SetPhaseTransitionInputLocked, false),
				PendingPhaseTitleDisplaySeconds,
				false
			);
		}
		else
		{
			SetPhaseTransitionInputLocked(false);
		}
	}
	else
	{
		SetPhaseTransitionInputLocked(false);
	}
}

void AA302GameHUD::SetPhaseTransitionOverlayOpacity(float Opacity)
{
	if (UBorder* BlackOverlay = FindPhaseTransitionBlackOverlay())
	{
		FLinearColor CurrentColor = BlackOverlay->GetBrushColor();
		CurrentColor.A = FMath::Clamp(Opacity, 0.0f, 1.0f);
		BlackOverlay->SetBrushColor(CurrentColor);
		return;
	}

	if (PhaseTransitionWidgetInstance)
	{
		PhaseTransitionWidgetInstance->SetRenderOpacity(FMath::Clamp(Opacity, 0.0f, 1.0f));
	}
}

void AA302GameHUD::SetPhaseTransitionInputLocked(bool bLocked)
{
	bPhaseTransitionInputLocked = bLocked;

	if (APlayerController* PC = GetOwningPlayerController())
	{
		PC->SetIgnoreMoveInput(bLocked);
		PC->SetIgnoreLookInput(bLocked);
	}
}

UBorder* AA302GameHUD::FindPhaseTransitionBlackOverlay() const
{
	return PhaseTransitionWidgetInstance
		? Cast<UBorder>(PhaseTransitionWidgetInstance->GetWidgetFromName(TEXT("BlackOverlay")))
		: nullptr;
}

void AA302GameHUD::ToggleInGameSettingMenu()
{
	if (PlayerHUDComponent) PlayerHUDComponent->ToggleInGameSettingMenu();
}

bool AA302GameHUD::IsInGameSettingMenuOpen() const
{
	return PlayerHUDComponent ? PlayerHUDComponent->IsInGameSettingMenuOpen() : false;
}

float AA302GameHUD::GetMouseSensitivityMultiplier() const
{
	return PlayerHUDComponent ? PlayerHUDComponent->GetMouseSensitivityMultiplier() : 1.0f;
}

void AA302GameHUD::ShowPublicMaliceAnnouncement(const FString& PlayerName, int32 MaliceCount)
{
	if (PlayerHUDComponent) PlayerHUDComponent->ShowPublicMaliceAnnouncement(PlayerName, MaliceCount);
}

void AA302GameHUD::ShowNotificationMessage(const FText& Message)
{
	UUserWidget* NotificationListWidget = GetOrCreateNotificationListWidget();
	if (!NotificationListWidget)
	{
		return;
	}

	if (UFunction* AddMessageFunction = NotificationListWidget->FindFunction(TEXT("AddNotificationMessage")))
	{
		struct FParams
		{
			FText NewMessage;
		};

		FParams Params{ Message };
		NotificationListWidget->ProcessEvent(AddMessageFunction, &Params);
	}
}

void AA302GameHUD::ShowInspectMaliceSelectionWidget()
{
	if (PlayerHUDComponent) PlayerHUDComponent->ShowInspectMaliceSelectionWidget();
}

void AA302GameHUD::ShowInspectMaliceSelectionWidgetWithConfig(float SelectionTimeoutSeconds, float ResultDisplaySeconds)
{
	if (PlayerHUDComponent)
	{
		PlayerHUDComponent->ShowInspectMaliceSelectionWidgetWithConfig(SelectionTimeoutSeconds, ResultDisplaySeconds);
	}
}

void AA302GameHUD::ShowInspectMaliceSelectionWidgetWithCandidatesAndConfig(const TArray<FInspectMaliceCandidateData>& Candidates, float SelectionTimeoutSeconds, float ResultDisplaySeconds)
{
	if (PlayerHUDComponent)
	{
		PlayerHUDComponent->ShowInspectMaliceSelectionWidgetWithCandidatesAndConfig(Candidates, SelectionTimeoutSeconds, ResultDisplaySeconds);
	}
}

void AA302GameHUD::UpdateShieldCountText(int32 ShieldCount)
{
	if (PlayerHUDComponent) PlayerHUDComponent->UpdateShieldCountText(ShieldCount);
}

void AA302GameHUD::UpdateMaliceCountText(int32 MaliceCount)
{
	if (PlayerHUDComponent) PlayerHUDComponent->UpdateMaliceCountText(MaliceCount);
}

void AA302GameHUD::UpdateItemTimerText(float RemainingSeconds)
{
	if (PlayerHUDComponent) PlayerHUDComponent->UpdateItemTimerText(RemainingSeconds);
}

void AA302GameHUD::SetItemTimerVisible(bool bVisible)
{
	if (PlayerHUDComponent) PlayerHUDComponent->SetItemTimerVisible(bVisible);
}

void AA302GameHUD::ConfigureMatchTimer(float MatchStartServerTime, float DurationSeconds, uint8 bVisibleInt)
{
	if (PlayerHUDComponent)
	{
		PlayerHUDComponent->ConfigureMatchTimer(MatchStartServerTime, DurationSeconds, bVisibleInt != 0);
	}
}

void AA302GameHUD::UpdatePhaseClearProgress(uint8 PhaseAsByte, int32 CurrentCount, int32 RequiredCount, uint8 bVisibleInt)
{
	if (PlayerHUDComponent)
	{
		PlayerHUDComponent->UpdatePhaseClearProgress(PhaseAsByte, CurrentCount, RequiredCount, bVisibleInt != 0);
	}
}

void AA302GameHUD::ShowPersonalEvent(FName EventID, const FText& EventTitle, const FText& EventDescription, const TArray<FText>& Choices)
{
	if (PlayerHUDComponent) PlayerHUDComponent->ShowPersonalEventUI(PersonalEventWidgetClass, EventID, EventTitle, EventDescription, Choices);
}

void AA302GameHUD::ShowGroupEventVote(FName EventID, const FText& EventTitle, const FText& EventDescription, float VoteDuration)
{
	if (PlayerHUDComponent) PlayerHUDComponent->ShowGroupEventVoteUI(GroupEventVoteWidgetClass, EventID, EventTitle, EventDescription, VoteDuration);
}

void AA302GameHUD::FinishGroupEventVoteUI(FName EventID, const FText& ResultText)
{
	if (PlayerHUDComponent) PlayerHUDComponent->FinishGroupEventVoteUI(EventID, ResultText);
}

void AA302GameHUD::ShowResultScreen(const FText& Title, const FText& Description, float DisplaySeconds)
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	if (!ResultWidgetClass)
	{
		if (UClass* LoadedResultClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WorkSpace/UI/WBP_Result.WBP_Result_C")))
		{
			ResultWidgetClass = LoadedResultClass;
		}
	}

	if (!ResultWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[A302GameHUD] Result widget class is missing."));
		return;
	}

	if (!ResultWidgetInstance)
	{
		ResultWidgetInstance = CreateWidget<UUserWidget>(PC, ResultWidgetClass);
	}

	if (!ResultWidgetInstance)
	{
		return;
	}

	auto SetFirstAvailableText = [&](const TArray<FName>& CandidateNames, const FText& InText)
	{
		for (const FName& CandidateName : CandidateNames)
		{
			if (UTextBlock* TextBlock = Cast<UTextBlock>(ResultWidgetInstance->GetWidgetFromName(CandidateName)))
			{
				TextBlock->SetText(InText);
				return;
			}
		}
	};

	SetFirstAvailableText({ TEXT("ResultTitle"), TEXT("TitleText"), TEXT("Txt_Title"), TEXT("EventTitle") }, Title);
	SetFirstAvailableText({ TEXT("ResultDescription"), TEXT("DescriptionText"), TEXT("Txt_Description"), TEXT("EventContext"), TEXT("BodyText") }, Description);
	SetFirstAvailableText({ TEXT("ResultCountdown"), TEXT("CountdownText"), TEXT("RemainTimeText") }, FText::AsNumber(FMath::Max(1, FMath::RoundToInt(DisplaySeconds))));

	if (!ResultWidgetInstance->IsInViewport())
	{
		ResultWidgetInstance->AddToViewport(300);
	}

	ResultWidgetInstance->SetVisibility(ESlateVisibility::Visible);

	if (ChatWidgetInstance)
	{
		ChatWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (TitleCardWidgetInstance)
	{
		TitleCardWidgetInstance->RemoveFromParent();
		TitleCardWidgetInstance = nullptr;
	}

	PC->SetInputMode(FInputModeUIOnly());
	PC->bShowMouseCursor = true;
	PC->bEnableClickEvents = true;
	PC->bEnableMouseOverEvents = true;
}

void AA302GameHUD::ShowDeathSpectatorUI()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	if (!DieWidgetClass)
	{
		if (UClass* LoadedDieClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WorkSpace/UI/WBP_Die.WBP_Die_C")))
		{
			DieWidgetClass = LoadedDieClass;
		}
	}

	if (!DieWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[A302GameHUD] Die widget class is missing."));
		return;
	}

	if (!DieWidgetInstance)
	{
		DieWidgetInstance = CreateWidget<UUserWidget>(PC, DieWidgetClass);
	}

	if (!DieWidgetInstance)
	{
		return;
	}

	if (!DieWidgetInstance->IsInViewport())
	{
		DieWidgetInstance->AddToViewport(290);
	}

	DieWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	SetDeathWidgetOtherPlayerSectionVisible(false);

	if (UWidget* YouDiedImage = DieWidgetInstance->GetWidgetFromName(TEXT("YouDiedImage")))
	{
		YouDiedImage->SetVisibility(ESlateVisibility::Visible);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DieWidgetIntroTimerHandle);
		World->GetTimerManager().SetTimer(
			DieWidgetIntroTimerHandle,
			this,
			&AA302GameHUD::HandleDeathWidgetIntroFinished,
			3.0f,
			false
		);
	}
}

void AA302GameHUD::HideDeathSpectatorUI()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DieWidgetIntroTimerHandle);
	}

	if (DieWidgetInstance)
	{
		DieWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void AA302GameHUD::UpdateDeathSpectatorTargetName(const FString& TargetPlayerName)
{
	if (!DieWidgetInstance)
	{
		return;
	}

	if (UTextBlock* OtherPlayerNameText = Cast<UTextBlock>(DieWidgetInstance->GetWidgetFromName(TEXT("OtherPlayerName"))))
	{
		OtherPlayerNameText->SetText(FText::FromString(TargetPlayerName));
	}
}

void AA302GameHUD::SetDeathWidgetOtherPlayerSectionVisible(bool bVisible)
{
	if (!DieWidgetInstance || !DieWidgetInstance->WidgetTree)
	{
		return;
	}

	TArray<UWidget*> AllWidgets;
	DieWidgetInstance->WidgetTree->GetAllWidgets(AllWidgets);
	for (UWidget* Widget : AllWidgets)
	{
		if (!Widget)
		{
			continue;
		}

		if (Widget->GetName().StartsWith(TEXT("OtherPlayer"), ESearchCase::CaseSensitive))
		{
			Widget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}
}

void AA302GameHUD::HandleDeathWidgetIntroFinished()
{
	if (!DieWidgetInstance)
	{
		return;
	}

	if (UWidget* YouDiedImage = DieWidgetInstance->GetWidgetFromName(TEXT("YouDiedImage")))
	{
		YouDiedImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	SetDeathWidgetOtherPlayerSectionVisible(true);
}

void AA302GameHUD::ShowEscapeWaitingUI()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	if (!EscapeWaitingWidgetClass)
	{
		if (UClass* LoadedClass = LoadClass<UEscapeWaitingWidget>(nullptr, TEXT("/Game/PersonalWorkSpace/sikk806/BP_EscapeWaitingWidget.BP_EscapeWaitingWidget_C")))
		{
			EscapeWaitingWidgetClass = LoadedClass;
		}
	}

	if (!EscapeWaitingWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[A302GameHUD] EscapeWaitingWidgetClass is missing."));
		return;
	}

	if (!EscapeWaitingWidgetInstance)
	{
		EscapeWaitingWidgetInstance = CreateWidget<UEscapeWaitingWidget>(PC, EscapeWaitingWidgetClass);
	}

	if (!EscapeWaitingWidgetInstance)
	{
		return;
	}

	if (!EscapeWaitingWidgetInstance->IsInViewport())
	{
		// DieWidget(290)보다 높은 Z-Order로 표시
		EscapeWaitingWidgetInstance->AddToViewport(291);
	}

	EscapeWaitingWidgetInstance->SetVisibility(ESlateVisibility::Visible);
}

void AA302GameHUD::HideEscapeWaitingUI()
{
	if (EscapeWaitingWidgetInstance)
	{
		EscapeWaitingWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void AA302GameHUD::UpdateEscapeSpectatorTargetName(const FString& TargetPlayerName)
{
	if (EscapeWaitingWidgetInstance)
	{
		EscapeWaitingWidgetInstance->UpdateSpectatorTargetName(TargetPlayerName);
	}
}
