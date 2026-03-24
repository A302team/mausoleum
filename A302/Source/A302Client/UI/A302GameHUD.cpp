#include "UI/A302GameHUD.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "UI/PersonalEventWidget.h"
#include "UI/PlayerHUDComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "Engine/World.h"

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

	static ConstructorHelpers::FClassFinder<UUserWidget> ResultWidgetBPClass(TEXT("/Game/WorkSpace/UI/WBP_Result"));
	if (ResultWidgetBPClass.Succeeded())
	{
		ResultWidgetClass = ResultWidgetBPClass.Class;
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

	InitializeChatWidget();

	if (PlayerHUDComponent)
	{
		PlayerHUDComponent->InitializeInGameHUD(QuickSlotBarClass, InGameSettingClass, InspectMaliceWidgetClass);
		PlayerHUDComponent->RefreshQuickSlotBinding();
	}
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
