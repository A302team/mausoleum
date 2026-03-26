#include "Character/MyPlayerController.h"

#include "Character/Components/PlayerEventComponent.h"
#include "Character/Components/Inventory/ItemManagerComponent.h"
// UI Widget 헤더들은 AA302GameHUD에서 관리합니다.
#include "EnhancedInputSubsystems.h"
#include "GameMode/A302GameState.h"
#include "GameMode/A302GameInstance.h"
#include "GameMode/A302PlayerState.h"
#include "GameData/Items/ItemDefinition.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "InputMappingContext.h"
#include "UObject/ConstructorHelpers.h"
#include "Character/Components/Audio/MaliceBGMComponent.h"      // Added
#include "Character/Components/Audio/GameBGMComponent.h"         // Added
#include "Character/Components/Audio/CursedSwordBGMComponent.h" // Added
#include "A302RuntimeGuards.h"
#include "GameFramework/HUD.h"
#include "Room/RoomScopeRules.h"
#include "Engine/World.h"

namespace
{
	constexpr int32 MaxVoiceRoomCodeRetryCount = 8;
	constexpr float PhaseTransitionFadeOutSeconds = 0.6f;
	constexpr float PhaseTransitionHoldSeconds = 0.5f;
	constexpr float PhaseTransitionFadeInSeconds = 0.6f;
	constexpr float PhaseTransitionTitleDisplaySeconds = 3.0f;

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

	bool IsLocalPieSession(const APlayerController* PlayerController)
	{
#if WITH_EDITOR
		if (!PlayerController)
		{
			return false;
		}

		const UWorld* World = PlayerController->GetWorld();
		return World && World->WorldType == EWorldType::PIE && World->GetNetMode() != NM_DedicatedServer;
#else
		return false;
#endif
	}

	FString BuildLocalPieRoomCode(const APlayerController* PlayerController)
	{
		const UWorld* World = PlayerController ? PlayerController->GetWorld() : nullptr;
		if (!World)
		{
			return FString();
		}

		FString SafeMapName = World->GetMapName();
		if (SafeMapName.StartsWith(TEXT("UEDPIE_")))
		{
			const int32 PrefixLen = 7;
			const int32 UnderscoreIndex = SafeMapName.Find(TEXT("_"), ESearchCase::IgnoreCase, ESearchDir::FromStart, PrefixLen);
			if (UnderscoreIndex != INDEX_NONE)
			{
				SafeMapName = SafeMapName.Mid(UnderscoreIndex + 1);
			}
		}

		for (TCHAR& Ch : SafeMapName)
		{
			if (!FChar::IsAlnum(Ch))
			{
				Ch = TEXT('_');
			}
		}

		return A302RoomScope::NormalizeRoomCode(FString::Printf(TEXT("PIE_LOCAL_%s"), *SafeMapName));
	}

	void BuildPhaseTransitionTexts(EGamePhase NewPhase, FText& OutTitle, FText& OutContext)
	{
		switch (NewPhase)
		{
		case EGamePhase::Phase0:
			OutTitle = FText::FromString(TEXT("제1장. 계시의 서곡"));
			OutContext = FText::FromString(TEXT("이제, 돌아갈 길은 없습니다."));
			break;
		case EGamePhase::Phase1:
			OutTitle = FText::FromString(TEXT("제2장. 의심의 성가"));
			OutContext = FText::FromString(TEXT("의심이 깊어질수록, 심판은 가까워집니다."));
			break;
		case EGamePhase::Phase2:
			OutTitle = FText::FromString(TEXT("제3장. 종말의 찬가"));
			OutContext = FText::FromString(TEXT("남은 것은 오직, 심판뿐입니다."));
			break;
		default:
			OutTitle = FText::GetEmpty();
			OutContext = FText::GetEmpty();
			break;
		}
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

void AMyPlayerController::TryRegisterPlayerDisplayName()
{
	if (!IsLocalController())
	{
		return;
	}

	const UA302GameInstance* GameInstance = GetGameInstance<UA302GameInstance>();
	if (!GameInstance)
	{
		return;
	}

	const FString DesiredName = GameInstance->MyPlayerName.TrimStartAndEnd();
	if (DesiredName.IsEmpty())
	{
		return;
	}

	const FString CurrentName = PlayerState ? PlayerState->GetPlayerName().TrimStartAndEnd() : FString();
	if (CurrentName.Equals(DesiredName, ESearchCase::CaseSensitive))
	{
		return;
	}

	Server_RegisterPlayerDisplayName(DesiredName);
}

AMyPlayerController::AMyPlayerController()
{
	PlayerEventComponent = CreateDefaultSubobject<UPlayerEventComponent>(TEXT("PlayerEventComponent"));

	// UI Widget 초기화 내용들은 AA302GameHUD로 이동되어 제거되었습니다.

	MaliceBGMComp = CreateDefaultSubobject<UMaliceBGMComponent>(TEXT("MaliceBGMComponent")); // Added
	GameBGMComp = CreateDefaultSubobject<UGameBGMComponent>(TEXT("GameBGMComponent"));       // Added
	CursedSwordBGMComp = CreateDefaultSubobject<UCursedSwordBGMComponent>(TEXT("CursedSwordBGMComponent")); // Added
}

void AMyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && IsLocalPieSession(this))
	{
		if (AA302PlayerState* A302PS = GetPlayerState<AA302PlayerState>())
		{
			const FString CurrentRoomCode = A302RoomScope::NormalizeRoomCode(A302PS->GetRoomCode());
			if (CurrentRoomCode.IsEmpty() || CurrentRoomCode == TEXT("DEFAULT"))
			{
				const FString FallbackRoomCode = BuildLocalPieRoomCode(this);
				if (!FallbackRoomCode.IsEmpty())
				{
					A302PS->SetRoomCode(FallbackRoomCode);
					UE_LOG(LogTemp, Log, TEXT("[RoomFallback] PIE local room code assigned. controller=%s room=%s"), *GetNameSafe(this), *FallbackRoomCode);
				}
			}

			if (!A302PS->bGameplayEnabled)
			{
				A302PS->SetGameplayEnabled(true);
				UE_LOG(LogTemp, Log, TEXT("[RoomFallback] PIE gameplay enabled forced. controller=%s"), *GetNameSafe(this));
			}
		}
	}

	if (!A302RuntimeGuards::ShouldInitializeLocalControllerUI(this))
	{
		return;
	}

	TryRegisterPlayerDisplayName();

	BindToReplicatedGamePhase();

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
	NotifyLocalGameplayPawnReady();

#if !UE_SERVER
	if (IsLocalController() && ShouldAttemptGameplayHUDInitialization())
	{
		bInGameHUDInitialized = false;
		TryInitializeInGameHUD();
	}
#endif
}

void AMyPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);
	DeadSpectateCycleIndex = INDEX_NONE;
	TryRegisterPlayerDisplayName();

	if (const AA302PlayerState* A302PlayerState = GetPlayerState<AA302PlayerState>())
	{
		if (A302PlayerState->bIsAlive)
		{
			HideDeathSpectatorUI();
		}
	}

#if !UE_SERVER
	if (IsLocalController() && ShouldAttemptGameplayHUDInitialization())
	{
		NotifyLocalGameplayPawnReady();
		TryInitializeInGameHUD();
	}
#endif
}


void AMyPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();
	DeadSpectateCycleIndex = INDEX_NONE;
	TryRegisterPlayerDisplayName();

	if (const AA302PlayerState* A302PlayerState = GetPlayerState<AA302PlayerState>())
	{
		if (A302PlayerState->bIsAlive)
		{
			HideDeathSpectatorUI();
		}
	}

	EnsureLocalVoiceComponent();

#if !UE_SERVER
	if (IsLocalController() && ShouldAttemptGameplayHUDInitialization())
	{
		TryInitializeInGameHUD();

		if (AHUD* CurrentHUD = GetHUD())
		{
			if (UFunction* Func = CurrentHUD->FindFunction(TEXT("RefreshQuickSlotBinding")))
			{
				CurrentHUD->ProcessEvent(Func, nullptr);
			}
		}
	}
#endif
}

bool AMyPlayerController::ShouldAttemptGameplayHUDInitialization() const
{
	return !A302RuntimeGuards::IsLobbyWorld(GetWorld());
}

void AMyPlayerController::TryInitializeInGameHUD()
{
#if !UE_SERVER
	if (!IsLocalController() || !ShouldAttemptGameplayHUDInitialization() || bInGameHUDInitialized)
	{
		return;
	}

	if (const UA302GameInstance* GameInstance = GetGameInstance<UA302GameInstance>())
	{
		if (!GameInstance->CanInitializeGameplayUI())
		{
			if (!GetWorldTimerManager().IsTimerActive(DeferredHUDInitTimerHandle))
			{
				GetWorldTimerManager().SetTimer(
					DeferredHUDInitTimerHandle,
					this,
					&AMyPlayerController::PollDeferredHUDInitialization,
					0.1f,
					true
				);
			}
			return;
		}
	}

	const TCHAR* HUDPath = TEXT("/Game/WorkSpace/UI/BP_A302GameHUD.BP_A302GameHUD_C");
	UClass* InGameHUDClass = LoadClass<AHUD>(nullptr, HUDPath);
	if (!InGameHUDClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load HUD class: %s"), HUDPath);
		return;
	}

	ClientSetHUD(InGameHUDClass);

	FTimerHandle HUDInitTimer;
	GetWorldTimerManager().SetTimer(HUDInitTimer, [this]()
	{
		if (AHUD* CurrentHUD = GetHUD())
		{
			if (UFunction* Func = CurrentHUD->FindFunction(TEXT("InitializeClientInGameWidgets")))
			{
				CurrentHUD->ProcessEvent(Func, nullptr);
				bInGameHUDInitialized = true;
				ApplyMatchTimerConfigToHUD();
				BindToReplicatedGamePhase();
				FlushQueuedPhaseTransition();
				FlushQueuedGameplayStartTitleCard();
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("InitializeClientInGameWidgets function not found on HUD: %s"), *GetNameSafe(CurrentHUD));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("HUD instance is null after attempting to set it."));
		}
	}, 0.2f, false);
#endif
}

void AMyPlayerController::ApplyMatchTimerConfigToHUD()
{
	if (!bHasPendingMatchTimerConfig)
	{
		return;
	}

	if (AHUD* GameHUD = GetHUD())
	{
		if (UFunction* Func = GameHUD->FindFunction(TEXT("ConfigureMatchTimer")))
		{
			struct FParams
			{
				float InMatchStartServerTime;
				float InDurationSeconds;
				uint8 bInVisible;
			};

			FParams Params
			{
				PendingMatchTimerStartServerTime,
				PendingMatchTimerDurationSeconds,
				static_cast<uint8>(bPendingMatchTimerVisible ? 1 : 0)
			};
			GameHUD->ProcessEvent(Func, &Params);
		}
	}
}

void AMyPlayerController::PollDeferredHUDInitialization()
{
#if !UE_SERVER
	if (!IsLocalController() || bInGameHUDInitialized)
	{
		GetWorldTimerManager().ClearTimer(DeferredHUDInitTimerHandle);
		return;
	}

	const UA302GameInstance* GameInstance = GetGameInstance<UA302GameInstance>();
	if (GameInstance && !GameInstance->CanInitializeGameplayUI())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(DeferredHUDInitTimerHandle);
	TryInitializeInGameHUD();
#endif
}

void AMyPlayerController::BindToReplicatedGamePhase()
{
	if (!IsLocalController())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AA302GameState* GameState = World->GetGameState<AA302GameState>();
	if (!GameState)
	{
		return;
	}

	if (BoundGameState.Get() == GameState)
	{
		return;
	}

	if (BoundGameState.IsValid())
	{
		BoundGameState->OnGamePhaseChanged().RemoveAll(this);
	}

	GameState->OnGamePhaseChanged().AddUObject(this, &AMyPlayerController::HandleReplicatedGamePhaseChanged);
	BoundGameState = GameState;
}

void AMyPlayerController::HandleReplicatedGamePhaseChanged(EGamePhase PreviousPhase, EGamePhase NewPhase, float PhaseChangedServerTime)
{
	if (!IsLocalController() || PreviousPhase == NewPhase || NewPhase == EGamePhase::Ended)
	{
		return;
	}

	QueuePhaseTransition(NewPhase, PhaseChangedServerTime);
	FlushQueuedPhaseTransition();
}

void AMyPlayerController::QueuePhaseTransition(EGamePhase NewPhase, float PhaseChangedServerTime)
{
	bHasQueuedPhaseTransition = true;
	QueuedPhaseTransition = NewPhase;
	QueuedPhaseChangedServerTime = PhaseChangedServerTime;
}

void AMyPlayerController::FlushQueuedPhaseTransition()
{
	if (!bHasQueuedPhaseTransition)
	{
		return;
	}

	AHUD* GameHUD = GetHUD();
	if (!GameHUD)
	{
		TryInitializeInGameHUD();
		return;
	}

	if (UFunction* Func = GameHUD->FindFunction(TEXT("StartPhaseTransition")))
	{
		FText PhaseTitle;
		FText PhaseContext;
		BuildPhaseTransitionTexts(QueuedPhaseTransition, PhaseTitle, PhaseContext);

		struct FParams
		{
			FText InTitle;
			FText InContext;
			float InFadeOutSeconds;
			float InHoldSeconds;
			float InFadeInSeconds;
			float InTitleDisplaySeconds;
		};

		FParams Params
		{
			PhaseTitle,
			PhaseContext,
			PhaseTransitionFadeOutSeconds,
			PhaseTransitionHoldSeconds,
			PhaseTransitionFadeInSeconds,
			PhaseTransitionTitleDisplaySeconds
		};
		GameHUD->ProcessEvent(Func, &Params);
		bHasQueuedPhaseTransition = false;
	}
}

void AMyPlayerController::ShowGameplayStartTitleCard()
{
	if (!IsLocalController())
	{
		return;
	}

	QueuedGameplayStartTitle = FText::FromString(TEXT("제1장. 계시의 서곡"));
	QueuedGameplayStartContext = FText::FromString(TEXT("이제, 돌아갈 길은 없습니다."));
	bHasQueuedGameplayStartTitleCard = true;
	FlushQueuedGameplayStartTitleCard();
}

void AMyPlayerController::FlushQueuedGameplayStartTitleCard()
{
	if (!bHasQueuedGameplayStartTitleCard)
	{
		return;
	}

	AHUD* GameHUD = GetHUD();
	if (!GameHUD)
	{
		TryInitializeInGameHUD();
		return;
	}

	if (UFunction* Func = GameHUD->FindFunction(TEXT("ShowTitleCard")))
	{
		struct FParams
		{
			FText InTitle;
			FText InContext;
			float InDisplaySeconds;
		};

		FParams Params
		{
			QueuedGameplayStartTitle,
			QueuedGameplayStartContext,
			PhaseTransitionTitleDisplaySeconds
		};

		GameHUD->ProcessEvent(Func, &Params);
		bHasQueuedGameplayStartTitleCard = false;
	}
}

void AMyPlayerController::NotifyLocalGameplayPawnReady()
{
	if (!IsLocalController())
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn || !ControlledPawn->HasActorBegunPlay())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || A302RuntimeGuards::IsLobbyWorld(World) || A302RuntimeGuards::IsDedicatedServerWorld(World))
	{
		return;
	}

	if (UA302GameInstance* GameInstance = GetGameInstance<UA302GameInstance>())
	{
		GameInstance->NotifyLocalGameplayPawnReady();
	}

	TryRegisterPlayerDisplayName();
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

void AMyPlayerController::ShowInspectMaliceSelectionWidgetWithConfig(float SelectionTimeoutSeconds, float ResultDisplaySeconds)
{
	if (AHUD* GameHUD = GetHUD())
	{
		if (UFunction* Func = GameHUD->FindFunction(TEXT("ShowInspectMaliceSelectionWidgetWithConfig")))
		{
			struct FParams
			{
				float InSelectionTimeoutSeconds;
				float InResultDisplaySeconds;
			};

			FParams Params
			{
				SelectionTimeoutSeconds,
				ResultDisplaySeconds
			};
			GameHUD->ProcessEvent(Func, &Params);
			return;
		}
	}

	ShowInspectMaliceSelectionWidget();
}

void AMyPlayerController::ShowInspectMaliceSelectionWidgetWithCandidatesAndConfig(const TArray<FInspectMaliceCandidateData>& Candidates, float SelectionTimeoutSeconds, float ResultDisplaySeconds)
{
	if (AHUD* GameHUD = GetHUD())
	{
		if (UFunction* Func = GameHUD->FindFunction(TEXT("ShowInspectMaliceSelectionWidgetWithCandidatesAndConfig")))
		{
			struct FParams
			{
				TArray<FInspectMaliceCandidateData> InCandidates;
				float InSelectionTimeoutSeconds;
				float InResultDisplaySeconds;
			};

			FParams Params
			{
				Candidates,
				SelectionTimeoutSeconds,
				ResultDisplaySeconds
			};
			GameHUD->ProcessEvent(Func, &Params);
			return;
		}
	}

	ShowInspectMaliceSelectionWidgetWithConfig(SelectionTimeoutSeconds, ResultDisplaySeconds);
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
	if (HasAuthority() && !IsLocalController())
	{
		Client_UpdateMaliceCount(MaliceCount);
		return;
	}

	if (AHUD* GameHUD = GetHUD())
	{
		if (UFunction* Func = GameHUD->FindFunction(TEXT("UpdateMaliceCountText")))
		{
			struct FParams { int32 Count; };
			FParams Params { MaliceCount };
			GameHUD->ProcessEvent(Func, &Params);
		}
	}
	// Added: Only play BGM on local player
	if (!IsLocalController())
	{
		return;
	}
	// End Added

	// Added: CursedSword BGM 상태 최우선 업데이트 — 검 보유 시 Malice 전환 차단
	if (CursedSwordBGMComp)
	{
		CursedSwordBGMComp->HandleMaliceState(MaliceCount);
	}
	// End Added

	// Added: 검 미보유 시에만 GameBGM ↔ MaliceBGM 전환 처리
	if (GameBGMComp && (!CursedSwordBGMComp || !CursedSwordBGMComp->HasCursedSword()))
	{
		GameBGMComp->HandleMaliceState(MaliceCount);
	}
	// End Added
}

void AMyPlayerController::UpdateItemTimer(float RemainingSeconds)
{
	if (HasAuthority() && !IsLocalController())
	{
		Client_UpdateItemTimer(RemainingSeconds);
		return;
	}

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
	if (HasAuthority() && !IsLocalController())
	{
		Client_SetItemTimerVisible(bVisible);
		return;
	}

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

void AMyPlayerController::ConfigureMatchTimer(float MatchStartServerTime, float DurationSeconds, bool bVisible)
{
	PendingMatchTimerStartServerTime = MatchStartServerTime;
	PendingMatchTimerDurationSeconds = DurationSeconds;
	bPendingMatchTimerVisible = bVisible;
	bHasPendingMatchTimerConfig = true;

	if (HasAuthority() && !IsLocalController())
	{
		Client_ConfigureMatchTimer(MatchStartServerTime, DurationSeconds, bVisible);
		return;
	}

	ApplyMatchTimerConfigToHUD();
}

void AMyPlayerController::UpdatePhaseClearProgress(uint8 PhaseAsByte, int32 CurrentCount, int32 RequiredCount, bool bVisible)
{
	if (HasAuthority() && !IsLocalController())
	{
		Client_UpdatePhaseClearProgress(PhaseAsByte, CurrentCount, RequiredCount, bVisible);
		return;
	}

	if (AHUD* GameHUD = GetHUD())
	{
		if (UFunction* Func = GameHUD->FindFunction(TEXT("UpdatePhaseClearProgress")))
		{
			struct FParams
			{
				uint8 InPhaseAsByte;
				int32 InCurrentCount;
				int32 InRequiredCount;
				uint8 bInVisible;
			};

			FParams Params
			{
				PhaseAsByte,
				CurrentCount,
				RequiredCount,
				static_cast<uint8>(bVisible ? 1 : 0)
			};
			GameHUD->ProcessEvent(Func, &Params);
		}
	}
}

void AMyPlayerController::ShowResultScreen(const FText& Title, const FText& Description, float DisplaySeconds)
{
	if (AHUD* GameHUD = GetHUD())
	{
		if (UFunction* Func = GameHUD->FindFunction(TEXT("ShowResultScreen")))
		{
			struct FParams
			{
				FText InTitle;
				FText InDescription;
				float InDisplaySeconds;
			};

			FParams Params { Title, Description, DisplaySeconds };
			GameHUD->ProcessEvent(Func, &Params);
		}
	}
}

void AMyPlayerController::ShowItemDescription(const FText& ItemName, const FText& Description, float DisplaySeconds)
{
	if (AHUD* GameHUD = GetHUD())
	{
		if (UFunction* Func = GameHUD->FindFunction(TEXT("ShowItemDescription")))
		{
			struct FParams
			{
				FText InItemName;
				FText InDescription;
				float InDisplaySeconds;
			};

			FParams Params { ItemName, Description, DisplaySeconds };
			GameHUD->ProcessEvent(Func, &Params);
		}
	}
}

void AMyPlayerController::ShowDeathSpectatorUI()
{
	if (AHUD* GameHUD = GetHUD())
	{
		if (UFunction* Func = GameHUD->FindFunction(TEXT("ShowDeathSpectatorUI")))
		{
			GameHUD->ProcessEvent(Func, nullptr);
		}
	}

	FString CurrentViewTargetName;
	if (const APawn* CurrentViewPawn = Cast<APawn>(GetViewTarget()))
	{
		if (const APlayerState* ViewPlayerState = CurrentViewPawn->GetPlayerState())
		{
			CurrentViewTargetName = ViewPlayerState->GetPlayerName();
		}

		if (CurrentViewTargetName.IsEmpty())
		{
			CurrentViewTargetName = CurrentViewPawn->GetName();
		}
	}

	UpdateDeathSpectatorTargetName(CurrentViewTargetName);
}

void AMyPlayerController::HideDeathSpectatorUI()
{
	if (AHUD* GameHUD = GetHUD())
	{
		if (UFunction* Func = GameHUD->FindFunction(TEXT("HideDeathSpectatorUI")))
		{
			GameHUD->ProcessEvent(Func, nullptr);
		}
	}
}

void AMyPlayerController::UpdateDeathSpectatorTargetName(const FString& TargetPlayerName)
{
	if (AHUD* GameHUD = GetHUD())
	{
		if (UFunction* Func = GameHUD->FindFunction(TEXT("UpdateDeathSpectatorTargetName")))
		{
			struct FParams
			{
				FString InTargetPlayerName;
			};

			FParams Params{ TargetPlayerName };
			GameHUD->ProcessEvent(Func, &Params);
		}
	}
}

void AMyPlayerController::CycleAlivePlayerViewTarget()
{
	if (!IsLocalController())
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	const AA302PlayerState* LocalPlayerState = GetPlayerState<AA302PlayerState>();
	if (!ControlledPawn || !LocalPlayerState || LocalPlayerState->bIsAlive)
	{
		return;
	}

	auto ResolveDisplayNameFromPawn = [](const APawn* InPawn) -> FString
	{
		if (!InPawn)
		{
			return FString();
		}

		if (const APlayerState* InPlayerState = InPawn->GetPlayerState())
		{
			const FString PlayerName = InPlayerState->GetPlayerName();
			if (!PlayerName.IsEmpty())
			{
				return PlayerName;
			}
		}

		return InPawn->GetName();
	};

	AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GameState)
	{
		return;
	}

	TArray<APawn*> AliveCandidatePawns;
	AliveCandidatePawns.Reserve(GameState->PlayerArray.Num());

	for (APlayerState* CandidatePlayerState : GameState->PlayerArray)
	{
		const AA302PlayerState* CandidateA302State = Cast<AA302PlayerState>(CandidatePlayerState);
		if (!CandidateA302State || !CandidateA302State->bIsAlive || CandidateA302State->bIsEscaped)
		{
			continue;
		}

		if (!A302RoomScope::ArePlayersInSameLogicalRoom(LocalPlayerState, CandidateA302State))
		{
			continue;
		}

		APawn* CandidatePawn = CandidatePlayerState ? CandidatePlayerState->GetPawn() : nullptr;
		if (!CandidatePawn || CandidatePawn == ControlledPawn)
		{
			continue;
		}

		AliveCandidatePawns.Add(CandidatePawn);
	}

	if (AliveCandidatePawns.Num() == 0)
	{
		DeadSpectateCycleIndex = INDEX_NONE;
		SetViewTarget(ControlledPawn);
		UpdateDeathSpectatorTargetName(ResolveDisplayNameFromPawn(ControlledPawn));
		return;
	}

	AliveCandidatePawns.Sort([](const APawn& LeftPawn, const APawn& RightPawn)
	{
		const APlayerState* LeftState = LeftPawn.GetPlayerState();
		const APlayerState* RightState = RightPawn.GetPlayerState();
		const int32 LeftPlayerId = LeftState ? LeftState->GetPlayerId() : MAX_int32;
		const int32 RightPlayerId = RightState ? RightState->GetPlayerId() : MAX_int32;
		return LeftPlayerId < RightPlayerId;
	});

	DeadSpectateCycleIndex = (DeadSpectateCycleIndex + 1) % AliveCandidatePawns.Num();
	APawn* SpectateTargetPawn = AliveCandidatePawns[DeadSpectateCycleIndex];
	SetViewTargetWithBlend(SpectateTargetPawn, 0.15f);
	UpdateDeathSpectatorTargetName(ResolveDisplayNameFromPawn(SpectateTargetPawn));
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

		if (RoomCode.IsEmpty())
		{
			if (UWorld* World = GetWorld())
			{
				if (VoiceRoomCodeRetryCount < MaxVoiceRoomCodeRetryCount)
				{
					++VoiceRoomCodeRetryCount;
					World->GetTimerManager().SetTimer(
						VoiceRoomCodeRetryTimerHandle,
						this,
						&AMyPlayerController::EnsureLocalVoiceComponent,
						0.5f,
						false
					);
				}
			}
		}
		else
		{
			VoiceRoomCodeRetryCount = 0;
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(VoiceRoomCodeRetryTimerHandle);
			}
		}
	}
}

void AMyPlayerController::Client_ShowInspectMaliceSelectionWidget_Implementation()
{
	ShowInspectMaliceSelectionWidget();
}

void AMyPlayerController::Client_ShowInspectMaliceSelectionWidgetWithConfig_Implementation(float SelectionTimeoutSeconds, float ResultDisplaySeconds)
{
	ShowInspectMaliceSelectionWidgetWithConfig(SelectionTimeoutSeconds, ResultDisplaySeconds);
}

void AMyPlayerController::Client_ShowInspectMaliceSelectionWidgetWithCandidatesAndConfig_Implementation(const TArray<FInspectMaliceCandidateData>& Candidates, float SelectionTimeoutSeconds, float ResultDisplaySeconds)
{
	ShowInspectMaliceSelectionWidgetWithCandidatesAndConfig(Candidates, SelectionTimeoutSeconds, ResultDisplaySeconds);
}

void AMyPlayerController::Client_ShowPublicMaliceAnnouncement_Implementation(const FString& PlayerName, int32 MaliceCount)
{
	ShowPublicMaliceAnnouncement(PlayerName, MaliceCount);
}

void AMyPlayerController::Client_UpdateItemTimer_Implementation(float RemainingSeconds)
{
	UpdateItemTimer(RemainingSeconds);
}

void AMyPlayerController::Client_UpdateMaliceCount_Implementation(int32 MaliceCount)
{
	UpdateMaliceCount(MaliceCount);
}

void AMyPlayerController::Client_SetItemTimerVisible_Implementation(bool bVisible)
{
	SetItemTimerVisibleForClient(bVisible);
}

void AMyPlayerController::Client_ConfigureMatchTimer_Implementation(float MatchStartServerTime, float DurationSeconds, bool bVisible)
{
	ConfigureMatchTimer(MatchStartServerTime, DurationSeconds, bVisible);
}

void AMyPlayerController::Client_UpdatePhaseClearProgress_Implementation(uint8 PhaseAsByte, int32 CurrentCount, int32 RequiredCount, bool bVisible)
{
	UpdatePhaseClearProgress(PhaseAsByte, CurrentCount, RequiredCount, bVisible);
}

void AMyPlayerController::Client_ShowResultScreen_Implementation(const FText& Title, const FText& Description, float DisplaySeconds)
{
	ShowResultScreen(Title, Description, DisplaySeconds);
}

void AMyPlayerController::Client_RemoveQuickSlotItemByServer_Implementation(int32 SlotIndex, FName ExpectedItemId)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	UItemManagerComponent* ItemManager = ControlledPawn->FindComponentByClass<UItemManagerComponent>();
	if (!ItemManager || !ItemManager->IsValidSlotIndex(SlotIndex))
	{
		return;
	}

	const UItemDefinition* CurrentItemDefinition = ItemManager->GetItemDefinitionAtSlot(SlotIndex);
	if (!CurrentItemDefinition)
	{
		return;
	}

	if (!ExpectedItemId.IsNone() && CurrentItemDefinition->ItemId != ExpectedItemId)
	{
		return;
	}

	ItemManager->RemoveItemFromSlot(SlotIndex);
}
