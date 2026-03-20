#include "UI/A302GameHUD.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
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

	static ConstructorHelpers::FClassFinder<UPersonalEventWidget> PersonalEventWidgetBPClass(TEXT("/Game/WorkSpace/UI/WBP_PersonalEvent.WBP_PersonalEvent_C"));
	if (PersonalEventWidgetBPClass.Succeeded())
	{
		PersonalEventWidgetClass = PersonalEventWidgetBPClass.Class;
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

	if (UTextBlock* TitleText = Cast<UTextBlock>(TitleCardWidgetInstance->GetWidgetFromName(TEXT("EventTitle"))))
	{
		TitleText->SetText(Title);
	}

	if (UTextBlock* ContextText = Cast<UTextBlock>(TitleCardWidgetInstance->GetWidgetFromName(TEXT("EventContext"))))
	{
		ContextText->SetText(Context);
	}

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
		TitleCardWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
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
