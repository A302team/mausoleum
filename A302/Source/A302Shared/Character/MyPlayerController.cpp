#include "Character/MyPlayerController.h"

#include "Character/Components/PlayerEventComponent.h"
#include "Character/Components/PlayerHUDComponent.h"
#include "Blueprint/UserWidget.h"
#include "Components/ActorComponent.h"
#include "Components/TextBlock.h"
#include "UI/PersonalEventWidget.h"
#include "EnhancedInputSubsystems.h"
#include "GameMode/A302PlayerState.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"
#include "A302RuntimeGuards.h"

namespace
{
	UClass* LoadClientVoiceComponentClass()
	{
		static TWeakObjectPtr<UClass> CachedClass;
		if (CachedClass.IsValid())
		{
			return CachedClass.Get();
		}

		UClass* LoadedClass = LoadClass<UActorComponent>(nullptr, TEXT("/Script/A302Client.PrivateVoiceChatComponent"));
		CachedClass = LoadedClass;
		return LoadedClass;
	}

	UActorComponent* FindComponentByClassPath(const APawn* Pawn, UClass* TargetClass)
	{
		if (!Pawn || !TargetClass)
		{
			return nullptr;
		}

		for (UActorComponent* Component : Pawn->GetComponents())
		{
			if (Component && Component->IsA(TargetClass))
			{
				return Component;
			}
		}

		return nullptr;
	}
}

void AMyPlayerController::Server_RegisterPlayerDisplayName_Implementation(const FString& DesiredName)
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
	PlayerEventComponent = CreateDefaultSubobject<UPlayerEventComponent>(TEXT("PlayerEventComponent"));
	PlayerHUDComponent = CreateDefaultSubobject<UPlayerHUDComponent>(TEXT("PlayerHUDComponent"));

	if (!QuickSlotBarClass)
	{
		QuickSlotBarClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WorkSpace/UI/WBP_HUD2.WBP_HUD2_C"));
	}

	if (!InGameSettingClass)
	{
		InGameSettingClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WorkSpace/UI/WBP_InGameSetting.WBP_InGameSetting_C"));
		if (!InGameSettingClass)
		{
			InGameSettingClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WorkSpace/UI/WBP_InGameSetting2.WBP_InGameSetting2_C"));
		}
	}

	if (!PersonalEventWidgetClass)
	{
		if (UClass* LoadedPersonalEventClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WorkSpace/UI/WBP_PersonalEvent.WBP_PersonalEvent_C")))
		{
			PersonalEventWidgetClass = LoadedPersonalEventClass;
		}
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
}

void AMyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!A302RuntimeGuards::ShouldInitializeLocalControllerUI(this))
	{
		return;
	}

	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (DefaultMappingContext)
			{
				Subsys->AddMappingContext(DefaultMappingContext, MappingPriority);
			}
		}
	}

	EnsureLocalVoiceComponent();

	if (!IsInGameMap())
	{
		return;
	}

	InitializeClientInGameWidgets();
}

void AMyPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	EnsureLocalVoiceComponent();
}

bool AMyPlayerController::IsInGameMap() const
{
	return A302RuntimeGuards::IsInGameWorld(this);
}

void AMyPlayerController::InitializeClientInGameWidgets()
{
	// Redirect fallback 대신 현재 실제 위젯 경로를 직접 사용한다.
	if (UClass* LoadedQuickSlotClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WorkSpace/UI/WBP_HUD2.WBP_HUD2_C")))
	{
		QuickSlotBarClass = LoadedQuickSlotClass;
	}

	if (UClass* LoadedInGameSettingClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WorkSpace/UI/WBP_InGameSetting.WBP_InGameSetting_C")))
	{
		InGameSettingClass = LoadedInGameSettingClass;
	}

	if (UClass* LoadedPersonalEventClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/WorkSpace/UI/WBP_PersonalEvent.WBP_PersonalEvent_C")))
	{
		PersonalEventWidgetClass = LoadedPersonalEventClass;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[MyPlayerController] InitializeClientInGameWidgets. controller=%s quickSlot=%s inGameSetting=%s personalEvent=%s"),
		*GetNameSafe(this),
		*GetNameSafe(QuickSlotBarClass.Get()),
		*GetNameSafe(InGameSettingClass.Get()),
		*GetNameSafe(PersonalEventWidgetClass.Get())
	);

	InitializeChatWidget();

	if (PlayerHUDComponent)
	{
		PlayerHUDComponent->InitializeInGameHUD(QuickSlotBarClass, InGameSettingClass, InspectMaliceWidgetClass);
		PlayerHUDComponent->RefreshQuickSlotBinding();
	}
}

void AMyPlayerController::InitializeChatWidget()
{
	if (ChatWidgetInstance)
	{
		return;
	}

	if (!ChatWidgetClass)
	{
		return;
	}

	ChatWidgetInstance = CreateWidget<UUserWidget>(this, ChatWidgetClass);
	if (!ChatWidgetInstance)
	{
		return;
	}

	ChatWidgetInstance->AddToViewport();
}

void AMyPlayerController::ToggleInGameSettingMenu()
{
	if (PlayerHUDComponent)
	{
		PlayerHUDComponent->ToggleInGameSettingMenu();
	}
}

bool AMyPlayerController::IsInGameSettingMenuOpen() const
{
	return PlayerHUDComponent ? PlayerHUDComponent->IsInGameSettingMenuOpen() : false;
}

float AMyPlayerController::GetMouseSensitivityMultiplier() const
{
	return PlayerHUDComponent ? PlayerHUDComponent->GetMouseSensitivityMultiplier() : 1.0f;
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
				&AMyPlayerController::HideTitleCard,
				FMath::Max(0.5f, DisplaySeconds),
				false
			);
		}
	}
}

void AMyPlayerController::ShowPublicMaliceAnnouncement(const FString& PlayerName, int32 MaliceCount)
{
	if (PlayerHUDComponent)
	{
		PlayerHUDComponent->ShowPublicMaliceAnnouncement(PlayerName, MaliceCount);
	}
}

void AMyPlayerController::SetActivePersonalEvent(UBaseEvent* Event)
{
	if (PlayerEventComponent)
	{
		PlayerEventComponent->SetActivePersonalEvent(Event);
	}
}

void AMyPlayerController::SetActiveGroupEvent(UBaseGroupEvent* Event)
{
	if (PlayerEventComponent)
	{
		PlayerEventComponent->SetActiveGroupEvent(Event);
	}
}

void AMyPlayerController::ShowPersonalEvent(FName EventID, const FText& Title, const FText& Description, const TArray<FText>& Choices)
{
	if (PlayerEventComponent)
	{
		PlayerEventComponent->ShowPersonalEvent(EventID, Title, Description, Choices);
	}
}

void AMyPlayerController::ShowInspectMaliceSelectionWidget()
{
	if (PlayerHUDComponent)
	{
		PlayerHUDComponent->ShowInspectMaliceSelectionWidget();
	}
}

void AMyPlayerController::OpenGroupEventVote(FName EventID, const FText& EventTitle, const FText& EventDescription, float VoteDuration)
{
	if (PlayerEventComponent)
	{
		PlayerEventComponent->OpenGroupEventVote(EventID, EventTitle, EventDescription, VoteDuration);
	}
}

void AMyPlayerController::FinishGroupEventVote(FName EventID, const FText& ResultText)
{
	if (PlayerEventComponent)
	{
		PlayerEventComponent->FinishGroupEventVote(EventID, ResultText);
	}
}

void AMyPlayerController::ApplyConfiscationToLocalInventory()
{
	if (PlayerEventComponent)
	{
		PlayerEventComponent->ApplyConfiscationToLocalInventory();
	}
}

void AMyPlayerController::UpdateShieldCount(int32 ShieldCount)
{
	if (PlayerHUDComponent)
	{
		PlayerHUDComponent->UpdateShieldCountText(ShieldCount);
	}
}

void AMyPlayerController::UpdateMaliceCount(int32 MaliceCount)
{
	if (PlayerHUDComponent)
	{
		PlayerHUDComponent->UpdateMaliceCountText(MaliceCount);
	}
}

void AMyPlayerController::UpdateItemTimer(float RemainingSeconds)
{
	if (PlayerHUDComponent)
	{
		PlayerHUDComponent->UpdateItemTimerText(RemainingSeconds);
	}
}

void AMyPlayerController::SetItemTimerVisibleForClient(bool bVisible)
{
	if (PlayerHUDComponent)
	{
		PlayerHUDComponent->SetItemTimerVisible(bVisible);
	}
}

void AMyPlayerController::ToggleVoiceChatCapture()
{
	EnsureLocalVoiceComponent();

	if (APawn* ControlledPawn = GetPawn())
	{
		if (UClass* VoiceComponentClass = LoadClientVoiceComponentClass())
		{
			if (UActorComponent* VoiceComponent = FindComponentByClassPath(ControlledPawn, VoiceComponentClass))
			{
				if (UFunction* ToggleFunction = VoiceComponent->FindFunction(TEXT("ToggleMicrophone")))
				{
					VoiceComponent->ProcessEvent(ToggleFunction, nullptr);
				}
			}
		}
	}
}

void AMyPlayerController::EnsureLocalVoiceComponent()
{
	if (!A302RuntimeGuards::ShouldInitializeLocalControllerUI(this))
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	UClass* VoiceComponentClass = LoadClientVoiceComponentClass();
	if (!VoiceComponentClass)
	{
		return;
	}

	UActorComponent* VoiceComponent = FindComponentByClassPath(ControlledPawn, VoiceComponentClass);
	if (!VoiceComponent)
	{
		VoiceComponent = NewObject<UActorComponent>(ControlledPawn, VoiceComponentClass, TEXT("PrivateVoiceChatComponent"));
		if (VoiceComponent)
		{
			ControlledPawn->AddInstanceComponent(VoiceComponent);
			VoiceComponent->RegisterComponent();
			UE_LOG(LogTemp, Log, TEXT("[Voice] Added local voice component to pawn: %s"), *GetNameSafe(ControlledPawn));
		}
	}

	if (!VoiceComponent)
	{
		return;
	}

	if (const AA302PlayerState* A302PlayerState = ControlledPawn->GetPlayerState<AA302PlayerState>())
	{
		const FString RoomCode = A302PlayerState->GetRoomCode();
		if (UFunction* SetRoomCodeFunction = VoiceComponent->FindFunction(TEXT("SetRoomCode")))
		{
			struct FSetRoomCodeParams
			{
				FString InRoomCode;
			};

			FSetRoomCodeParams Params;
			Params.InRoomCode = RoomCode;
			VoiceComponent->ProcessEvent(SetRoomCodeFunction, &Params);
		}
	}
}

void AMyPlayerController::Client_ShowInspectMaliceSelectionWidget_Implementation()
{
	ShowInspectMaliceSelectionWidget();
}
