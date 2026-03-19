#include "Character/MyPlayerController.h"

#include "Character/Components/PlayerEventComponent.h"
// UI Widget 헤더들은 AA302GameHUD에서 관리합니다.
#include "EnhancedInputSubsystems.h"
#include "GameMode/A302PlayerState.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"
#include "A302RuntimeGuards.h"
#include "GameFramework/HUD.h"

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

	PlayerState->SetPlayerName(TrimmedName.Left(20));
}

AMyPlayerController::AMyPlayerController()
{
	PlayerEventComponent = CreateDefaultSubobject<UPlayerEventComponent>(TEXT("PlayerEventComponent"));

	// UI Widget 초기화 내용들은 AA302GameHUD로 이동되어 제거되었습니다.
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

#if !UE_SERVER
	if (IsLocalController() && IsInGameMap())
	{
		UE_LOG(LogTemp, Warning, TEXT("[UI/Debug] BeginPlay - IsLocal: 1, IsInGame: 1, Map: %s"), *GetWorld()->GetMapName());
		
		FString HUDPath = TEXT("/Game/WorkSpace/UI/BP_A302GameHUD.BP_A302GameHUD_C");
		UClass* InGameHUDClass = LoadClass<AHUD>(nullptr, *HUDPath);
		if (InGameHUDClass)
		{
			ClientSetHUD(InGameHUDClass);
			UE_LOG(LogTemp, Warning, TEXT("[UI/Debug] Successfully loaded and set HUD class: %s"), *HUDPath);
		
			FTimerHandle HUDInitTimer;
			GetWorldTimerManager().SetTimer(HUDInitTimer, [this]()
			{
				if (AHUD* CurrentHUD = GetHUD())
				{
					UE_LOG(LogTemp, Warning, TEXT("[UI/Debug] Current HUD Instance: %s"), *CurrentHUD->GetName());
					if (UFunction* Func = CurrentHUD->FindFunction(TEXT("InitializeClientInGameWidgets")))
					{
						UE_LOG(LogTemp, Warning, TEXT("[UI/Debug] Found InitializeClientInGameWidgets function. Calling..."));
						CurrentHUD->ProcessEvent(Func, nullptr);
					}
					else
					{
						UE_LOG(LogTemp, Error, TEXT("[UI/Debug] InitializeClientInGameWidgets function NOT FOUND on HUD: %s"), *GetNameSafe(CurrentHUD));
					}
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[UI/Debug] HUD Instance is NULL after attempt to set it."));
				}
			}, 0.2f, false);
		}
	}
#endif
}

void AMyPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

#if !UE_SERVER
	if (IsLocalController() && IsInGameMap())
	{
		if (AHUD* CurrentHUD = GetHUD())
		{
			if (UFunction* Func = CurrentHUD->FindFunction(TEXT("InitializeClientInGameWidgets")))
			{
				UE_LOG(LogTemp, Warning, TEXT("[UI/Debug] AcknowledgePossession - Refreshing HUD for new Pawn."));
			}
		}
	}
#endif
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

// InitializeClientInGameWidgets, InitializeChatWidget은 AA302GameHUD로 이관되어 제거되었습니다.

void AMyPlayerController::ToggleInGameSettingMenu()
{
	if (AHUD* GameHUD = GetHUD())
	{
		if (UFunction* Func = GameHUD->FindFunction(TEXT("ToggleInGameSettingMenu")))
		{
			GameHUD->ProcessEvent(Func, nullptr);
		}
	}
}

bool AMyPlayerController::IsInGameSettingMenuOpen() const
{
	if (AHUD* GameHUD = GetHUD())
	{
		if (UFunction* Func = GameHUD->FindFunction(TEXT("IsInGameSettingMenuOpen")))
		{
			bool bResult = false;
			GameHUD->ProcessEvent(Func, &bResult);
			return bResult;
		}
	}
	return false;
}

float AMyPlayerController::GetMouseSensitivityMultiplier() const
{
	if (AHUD* GameHUD = GetHUD())
	{
		if (UFunction* Func = GameHUD->FindFunction(TEXT("GetMouseSensitivityMultiplier")))
		{
			float Result = 1.0f;
			GameHUD->ProcessEvent(Func, &Result);
			return Result;
		}
	}
	return 1.0f;
}

void AMyPlayerController::Client_HideTitleCard_Implementation()
{
	if (AHUD* GameHUD = GetHUD())
	{
		if (UFunction* HideFunc = GameHUD->FindFunction(TEXT("HideTitleCard")))
		{
			GameHUD->ProcessEvent(HideFunc, nullptr);
		}
	}
}

void AMyPlayerController::Client_ReceiveSystemMessage_Implementation(const FString& SystemMessage)
{
	UE_LOG(LogTemp, Warning, TEXT("[SystemMessage]: %s"), *SystemMessage);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, SystemMessage);
	}
}

void AMyPlayerController::Client_ShowTitleCard_Implementation(const FText& Title, const FText& Context, float DisplaySeconds)
{
	if (AHUD* GameHUD = GetHUD())
	{
		if (UFunction* ShowFunc = GameHUD->FindFunction(TEXT("ShowTitleCard")))
		{
			struct FShowCardParams { FText InTitle; FText InContext; float InDisplaySeconds; };
			FShowCardParams Params;
			Params.InTitle = Title;
			Params.InContext = Context;
			Params.InDisplaySeconds = DisplaySeconds;
			GameHUD->ProcessEvent(ShowFunc, &Params);
		}
	}
}

void AMyPlayerController::ShowPublicMaliceAnnouncement(const FString& PlayerName, int32 MaliceCount)
{
	if (AHUD* GameHUD = GetHUD())
	{
		if (UFunction* Func = GameHUD->FindFunction(TEXT("ShowPublicMaliceAnnouncement")))
		{
			struct FParams { FString InName; int32 InCount; };
			FParams Params { PlayerName, MaliceCount };
			GameHUD->ProcessEvent(Func, &Params);
		}
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
		PlayerEventComponent->SetActiveGroupEvent(Event);
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
	if (AHUD* GameHUD = GetHUD())
	{
		if (UFunction* Func = GameHUD->FindFunction(TEXT("ShowInspectMaliceSelectionWidget")))
		{
			GameHUD->ProcessEvent(Func, nullptr);
		}
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
	if (AHUD* GameHUD = GetHUD())
	{
		if (UFunction* Func = GameHUD->FindFunction(TEXT("UpdateShieldCountText")))
		{
			struct FParams { int32 Count; };
			FParams Params { ShieldCount };
			GameHUD->ProcessEvent(Func, &Params);
		}
	}
}

void AMyPlayerController::UpdateMaliceCount(int32 MaliceCount)
{
	if (AHUD* GameHUD = GetHUD())
	{
		if (UFunction* Func = GameHUD->FindFunction(TEXT("UpdateMaliceCountText")))
		{
			struct FParams { int32 Count; };
			FParams Params { MaliceCount };
			GameHUD->ProcessEvent(Func, &Params);
		}
	}
}

void AMyPlayerController::UpdateItemTimer(float RemainingSeconds)
{
	if (AHUD* GameHUD = GetHUD())
	{
		if (UFunction* Func = GameHUD->FindFunction(TEXT("UpdateItemTimerText")))
		{
			struct FParams { float Seconds; };
			FParams Params { RemainingSeconds };
			GameHUD->ProcessEvent(Func, &Params);
		}
	}
}

void AMyPlayerController::SetItemTimerVisibleForClient(bool bVisible)
{
	if (AHUD* GameHUD = GetHUD())
	{
		if (UFunction* Func = GameHUD->FindFunction(TEXT("SetItemTimerVisible")))
		{
			struct FParams { bool bVis; };
			FParams Params { bVisible };
			GameHUD->ProcessEvent(Func, &Params);
		}
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

void AMyPlayerController::Client_ShowPublicMaliceAnnouncement_Implementation(const FString& PlayerName, int32 MaliceCount)
{
	ShowPublicMaliceAnnouncement(PlayerName, MaliceCount);
}
