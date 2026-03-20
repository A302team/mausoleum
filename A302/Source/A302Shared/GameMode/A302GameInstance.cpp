// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/A302GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "A302RuntimeGuards.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/World.h"
#include "Room/RoomWorldOffset.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	constexpr TCHAR LoadingWidgetClassPath[] = TEXT("/Game/WorkSpace/UI/WBP_Loading.WBP_Loading_C");
	constexpr const TCHAR* LoadingProgressFunctionCandidates[] = {
		TEXT("SetLoadingProgress"),
		TEXT("UpdateLoadingProgress")
	};
	constexpr const TCHAR* LoadingProgressBarNameCandidates[] = {
		TEXT("LoadingProgress"),
		TEXT("LoadingProgressBar"),
		TEXT("ProgressBar_Loading"),
		TEXT("PB_LoadingProgress"),
		TEXT("ProgressBar")
	};
}

UA302GameInstance::UA302GameInstance()
{
	LoadingWidgetClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(LoadingWidgetClassPath));
}

void UA302GameInstance::Init()
{
	Super::Init();

	if (A302RuntimeGuards::IsDedicatedServerWorld(this))
	{
		UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] Dedicated Server detected. Skipping Client Network/UI Initialization."));
		return;
	}

	// Client-only: Load Network Subsystem dynamically to avoid module hard dependency
	if (UClass* NetworkSubsystemClass = LoadClass<UObject>(nullptr, TEXT("/Script/A302Client.GameNetworkSubsystem")))
	{
		GameNetworkSubsystem = GetSubsystemBase(NetworkSubsystemClass);
		if (GameNetworkSubsystem)
		{
			// Connect to server using ProcessEvent to avoid linking A302Client symbols
			if (UFunction* ConnectFunc = GameNetworkSubsystem->FindFunction(TEXT("Connect")))
			{
				struct FConnectParams { uint8 Protocol; FString URL; };
				FConnectParams Params;
				Params.Protocol = 0; // WebSocket

				// GetLobbyURL via ProcessEvent
				if (UFunction* GetLobbyURLFunc = GameNetworkSubsystem->FindFunction(TEXT("GetLobbyURL")))
				{
					FString LobbyURL;
					GameNetworkSubsystem->ProcessEvent(GetLobbyURLFunc, &LobbyURL);
					Params.URL = LobbyURL;
				}
				
				GameNetworkSubsystem->ProcessEvent(ConnectFunc, &Params);
			}

			// Add PacketReceived callback via generic dynamic delegate binding
			if (FProperty* PacketReceivedProp = GameNetworkSubsystem->GetClass()->FindPropertyByName(TEXT("OnPacketReceived")))
			{
				if (FMulticastDelegateProperty* DelegateProp = CastField<FMulticastDelegateProperty>(PacketReceivedProp))
				{
					FScriptDelegate Delegate;
					Delegate.BindUFunction(this, TEXT("OnMessageReceived"));
					DelegateProp->AddDelegate(Delegate, GameNetworkSubsystem);
				}
			}
		}
	}

	// Client-only: Initialize MessageRouter
	if (UClass* RouterClass = LoadClass<UObject>(nullptr, TEXT("/Script/A302Client.LobbyMessageRouter")))
	{
		MessageRouter = NewObject<UObject>(this, RouterClass);
		if (MessageRouter)
		{
			if (UFunction* RouterInitFunc = MessageRouter->FindFunction(TEXT("Initialize")))
			{
				UA302GameInstance* ThisPtr = this;
				MessageRouter->ProcessEvent(RouterInitFunc, &ThisPtr);
			}
		}
	}

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UA302GameInstance::OnMapLoaded);
}

void UA302GameInstance::OnStart()
{
	Super::OnStart();

	if (A302RuntimeGuards::IsDedicatedServerWorld(this))
		return;

	if (UWorld* CurrentWorld = GetWorld())
	{
		OnMapLoaded(CurrentWorld);
	}
}

bool UA302GameInstance::CanInitializeGameplayUI() const
{
	UWorld* World = GetWorld();
	if (!World || A302RuntimeGuards::IsDedicatedServerWorld(World) || A302RuntimeGuards::IsLobbyWorld(World))
	{
		return false;
	}

	return !bWaitingForRoomStreamingReady;
}

void UA302GameInstance::OnMapLoaded(UWorld* LoadedWorld)
{
	if (!LoadedWorld || LoadedWorld != GetWorld() || A302RuntimeGuards::IsDedicatedServerWorld(LoadedWorld))
		return;

	FString MapName = LoadedWorld->GetMapName();
	UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] Map Loaded: %s"), *MapName);

	LoadedWorld->GetTimerManager().ClearTimer(LobbyWidgetRetryTimerHandle);
	LoadedWorld->GetTimerManager().ClearTimer(LoadingWidgetRetryTimerHandle);
	LoadedWorld->GetTimerManager().ClearTimer(StartGameLoadingTransitionTimeoutTimerHandle);
	LoadedWorld->GetTimerManager().ClearTimer(FinalizeLoadingProgressTimerHandle);
	LoadedWorld->GetTimerManager().ClearTimer(RoomStreamingReadyPollTimerHandle);
	bStartGameLoadingTransitionActive = false;

	if (A302RuntimeGuards::IsLoadingWorld(LoadedWorld))
	{
		SetLoadingProgressValue(0.35f);
		LoadingProgressPhaseStartTime = LoadedWorld->GetTimeSeconds();
		const FString ResolvedRoomCode = ResolveRoomCodeForInGameWorld(LoadedWorld);

		if (LobbyWidget)
		{
			LobbyWidget->RemoveFromParent();
			LobbyWidget = nullptr;
		}
		if (WaitingRoomWidget)
		{
			WaitingRoomWidget->RemoveFromParent();
			WaitingRoomWidget = nullptr;
		}

		LocalRoomStreamingLevel = nullptr;
		LocalRoomStreamingRoomCode.Reset();
		bWaitingForRoomStreamingReady = !ResolvedRoomCode.IsEmpty();

		if (!bWaitingForRoomStreamingReady)
		{
			UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] Loading world opened without room code. Skipping local room streaming wait."));
			HideLoadingWidget();
			return;
		}

		if (!TryCreateLoadingWidget(LoadedWorld))
		{
			LoadedWorld->GetTimerManager().SetTimer(
				LoadingWidgetRetryTimerHandle,
				FTimerDelegate::CreateLambda([this, LoadedWorld]()
				{
					TryCreateLoadingWidget(LoadedWorld);
				}),
				0.1f,
				true
			);
		}

		EnsureLocalRoomLevelInstance(LoadedWorld);
		SetLoadingProgressValue(0.55f);
		LoadedWorld->GetTimerManager().SetTimer(
			RoomStreamingReadyPollTimerHandle,
			this,
			&UA302GameInstance::PollRoomStreamingReady,
			0.1f,
			true
		);
		return;
	}

	// Remove Lobby UI when entering in-game
	if (A302RuntimeGuards::IsInGameWorld(LoadedWorld))
	{
		if (LobbyWidget)
		{
			LobbyWidget->RemoveFromParent();
			LobbyWidget = nullptr;
		}
		if (WaitingRoomWidget)
		{
			WaitingRoomWidget->RemoveFromParent();
			WaitingRoomWidget = nullptr;
		}
		return;
	}

	if (LocalRoomStreamingLevel)
	{
		LocalRoomStreamingLevel->SetIsRequestingUnloadAndRemoval(true);
		LocalRoomStreamingLevel = nullptr;
		LocalRoomStreamingRoomCode.Reset();
	}

	HideLoadingWidget();

	// Create Lobby Widget only in Lobby World
	if (!A302RuntimeGuards::IsLobbyWorld(LoadedWorld))
		return;

	if (!TryCreateLobbyWidget(LoadedWorld))
	{
		LoadedWorld->GetTimerManager().SetTimer(
			LobbyWidgetRetryTimerHandle,
			FTimerDelegate::CreateLambda([this, LoadedWorld]() { TryCreateLobbyWidget(LoadedWorld); }),
			0.1f,
			true
		);
	}
}

bool UA302GameInstance::TryCreateLobbyWidget(UWorld* LoadedWorld)
{
	if (!LoadedWorld || LoadedWorld != GetWorld() || A302RuntimeGuards::IsDedicatedServerWorld(LoadedWorld))
		return false;

	if (LobbyWidgetClass.IsNull())
		return false;

	APlayerController* PC = LoadedWorld->GetFirstPlayerController();
	if (!PC)
		return false;

	if (LobbyWidget)
	{
		LobbyWidget->SetVisibility(ESlateVisibility::Visible);
		LoadedWorld->GetTimerManager().ClearTimer(LobbyWidgetRetryTimerHandle);
		return true;
	}

	PC->bShowMouseCursor = true;
	PC->bEnableClickEvents = true;
	PC->SetInputMode(FInputModeUIOnly());

	UClass* LoadedClass = LobbyWidgetClass.LoadSynchronous();
	if (!LoadedClass) return false;

	LobbyWidget = CreateWidget<UUserWidget>(PC, LoadedClass);
	if (LobbyWidget)
	{
		LobbyWidget->AddToViewport();
		LoadedWorld->GetTimerManager().ClearTimer(LobbyWidgetRetryTimerHandle);
		UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] LobbyWidget created successfully."));
		return true;
	}

	return false;
}

bool UA302GameInstance::TryCreateLoadingWidget(UWorld* LoadedWorld)
{
	if (!LoadedWorld || LoadedWorld != GetWorld() || A302RuntimeGuards::IsDedicatedServerWorld(LoadedWorld))
	{
		return false;
	}

	if (LoadingWidget && !IsValid(LoadingWidget))
	{
		LoadingWidget = nullptr;
	}

	if (LoadingWidget)
	{
		if (!LoadingWidget->IsInViewport())
		{
			LoadingWidget->AddToViewport(5000);
			UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] Re-attached loading widget to viewport after map transition."));
		}

		LoadingWidget->SetVisibility(ESlateVisibility::Visible);
		PushLoadingProgressToWidget();
		LoadedWorld->GetTimerManager().ClearTimer(LoadingWidgetRetryTimerHandle);
		return true;
	}

	TSubclassOf<UUserWidget> LoadedClass = ResolveLoadingWidgetClass();
	if (!LoadedClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameInstance] Loading widget class is not configured."));
		return false;
	}

	APlayerController* PC = LoadedWorld->GetFirstPlayerController();
	if (!PC)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[GameMode/A302GameInstance] Loading widget owner PC is not ready yet. Falling back to world-owned widget creation."));
	}

	if (PC)
	{
		PC->SetInputMode(FInputModeUIOnly());
		PC->bShowMouseCursor = false;
		PC->bEnableClickEvents = false;
		LoadingWidget = CreateWidget<UUserWidget>(PC, LoadedClass);
	}
	else
	{
		LoadingWidget = CreateWidget<UUserWidget>(LoadedWorld, LoadedClass);
	}

	if (!LoadingWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameInstance] Failed to create loading widget instance."));
		return false;
	}

	LoadingWidget->AddToViewport(5000);
	PushLoadingProgressToWidget();
	LoadedWorld->GetTimerManager().ClearTimer(LoadingWidgetRetryTimerHandle);
	UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] Loading widget created. owner=%s"), PC ? TEXT("PlayerController") : TEXT("World"));
	return true;
}

void UA302GameInstance::SetLoadingProgressValue(float NewProgress)
{
	const float ClampedProgress = FMath::Clamp(NewProgress, 0.0f, 1.0f);
	if (FMath::IsNearlyEqual(LoadingProgress, ClampedProgress, KINDA_SMALL_NUMBER))
	{
		return;
	}

	LoadingProgress = ClampedProgress;
	PushLoadingProgressToWidget();
}

void UA302GameInstance::PushLoadingProgressToWidget() const
{
	if (!LoadingWidget)
	{
		return;
	}

	for (const TCHAR* FunctionName : LoadingProgressFunctionCandidates)
	{
		if (UFunction* SetProgressFunc = LoadingWidget->FindFunction(FunctionName))
		{
			struct FSetLoadingProgressParams
			{
				float Progress;
			};

			FSetLoadingProgressParams Params { LoadingProgress };
			LoadingWidget->ProcessEvent(SetProgressFunc, &Params);
			return;
		}
	}

	for (const TCHAR* ProgressBarName : LoadingProgressBarNameCandidates)
	{
		if (UProgressBar* ProgressBar = Cast<UProgressBar>(LoadingWidget->GetWidgetFromName(ProgressBarName)))
		{
			ProgressBar->SetPercent(LoadingProgress);
			return;
		}

		if (FObjectProperty* ProgressBarProperty = FindFProperty<FObjectProperty>(LoadingWidget->GetClass(), ProgressBarName))
		{
			UObject* ProgressBarObject = ProgressBarProperty->GetObjectPropertyValue_InContainer(LoadingWidget);
			if (UProgressBar* ProgressBar = Cast<UProgressBar>(ProgressBarObject))
			{
				ProgressBar->SetPercent(LoadingProgress);
				return;
			}
		}
	}
}

void UA302GameInstance::HideLoadingWidget()
{
	LoadingProgress = 0.0f;
	if (LoadingWidget)
	{
		LoadingWidget->RemoveFromParent();
		LoadingWidget = nullptr;
	}
}

void UA302GameInstance::PollRoomStreamingReady()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!bWaitingForRoomStreamingReady)
	{
		World->GetTimerManager().ClearTimer(RoomStreamingReadyPollTimerHandle);
		return;
	}

	if (!A302RuntimeGuards::IsLoadingWorld(World))
	{
		World->GetTimerManager().ClearTimer(RoomStreamingReadyPollTimerHandle);
		return;
	}
	const float ElapsedSinceStreamingStarted = FMath::Max(World->GetTimeSeconds() - LoadingProgressPhaseStartTime, 0.0f);
	const float NormalizedProgress = FMath::Clamp(ElapsedSinceStreamingStarted / 6.0f, 0.0f, 1.0f);
	const float SmoothedProgress = FMath::InterpEaseInOut(0.55f, 0.88f, NormalizedProgress, 2.0f);
	SetLoadingProgressValue(FMath::Max(LoadingProgress, SmoothedProgress));

	if (!LocalRoomStreamingLevel)
	{
		if (!ResolveRoomCodeForInGameWorld(World).IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameInstance] Waiting for room streaming, but local room level was not created yet."));
		}
		return;
	}

	if (!LocalRoomStreamingLevel->HasLoadedLevel() || !LocalRoomStreamingLevel->IsLevelVisible())
	{
		return;
	}

	if (!World->GetTimerManager().IsTimerActive(FinalizeLoadingProgressTimerHandle))
	{
		FinalizeLoadingProgressStartTime = World->GetTimeSeconds();
		FinalizeLoadingProgressStartValue = FMath::Max(LoadingProgress, 0.9f);
		SetLoadingProgressValue(FinalizeLoadingProgressStartValue);
		World->GetTimerManager().ClearTimer(RoomStreamingReadyPollTimerHandle);
		World->GetTimerManager().SetTimer(
			FinalizeLoadingProgressTimerHandle,
			this,
			&UA302GameInstance::FinalizeLoadingProgress,
			0.05f,
			true
		);
	}
	UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] Local room streaming level loaded. Finalizing loading progress. room=%s"), *LocalRoomStreamingRoomCode);
}

void UA302GameInstance::FinalizeLoadingProgress()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float Elapsed = FMath::Max(World->GetTimeSeconds() - FinalizeLoadingProgressStartTime, 0.0f);
	const float Alpha = FMath::Clamp(Elapsed / 0.45f, 0.0f, 1.0f);
	const float SmoothedAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
	SetLoadingProgressValue(FMath::Lerp(FinalizeLoadingProgressStartValue, 1.0f, SmoothedAlpha));

	if (Alpha < 1.0f)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(FinalizeLoadingProgressTimerHandle);
	HideLoadingWidget();
	bWaitingForRoomStreamingReady = false;
	if (APlayerController* LocalPC = World->GetFirstPlayerController())
	{
		LocalPC->SetInputMode(FInputModeGameOnly());
		LocalPC->bShowMouseCursor = false;
		LocalPC->bEnableClickEvents = false;
	}
	UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] Local room streaming level is ready. room=%s"), *LocalRoomStreamingRoomCode);
}

TSubclassOf<UUserWidget> UA302GameInstance::ResolveLoadingWidgetClass()
{
	if (!LoadingWidgetClass.IsNull())
	{
		if (UClass* LoadedClass = LoadingWidgetClass.LoadSynchronous())
		{
			return LoadedClass;
		}
	}

	if (UClass* FallbackClass = LoadClass<UUserWidget>(nullptr, LoadingWidgetClassPath))
	{
		return FallbackClass;
	}

	return nullptr;
}

void UA302GameInstance::ConnectToServer(const FString& URL)
{
	if (!GameNetworkSubsystem) return;

	if (UFunction* GetLobbyURLFunc = GameNetworkSubsystem->FindFunction(TEXT("GetLobbyURL")))
	{
		FString TargetURL;
		GameNetworkSubsystem->ProcessEvent(GetLobbyURLFunc, &TargetURL);
		
		if (UFunction* ConnectFunc = GameNetworkSubsystem->FindFunction(TEXT("Connect")))
		{
			struct FConnectParams { uint8 Protocol; FString URL; };
			FConnectParams Params { 0, TargetURL };
			GameNetworkSubsystem->ProcessEvent(ConnectFunc, &Params);
		}
	}
}

void UA302GameInstance::SendToServer(const FString& Message)
{
	if (GameNetworkSubsystem)
	{
		if (UFunction* SendFunc = GameNetworkSubsystem->FindFunction(TEXT("SendPacket")))
		{
			struct FParams { uint8 Protocol; FString Msg; };
			FParams Params { 0, Message };
			GameNetworkSubsystem->ProcessEvent(SendFunc, &Params);
		}
	}
}

void UA302GameInstance::BeginStartGameLoadingTransition(float TimeoutSeconds)
{
	UWorld* World = GetWorld();
	if (!World || A302RuntimeGuards::IsDedicatedServerWorld(World) || !A302RuntimeGuards::IsLobbyWorld(World))
	{
		return;
	}

	bStartGameLoadingTransitionActive = true;
	SetLoadingProgressValue(0.05f);
	TryCreateLoadingWidget(World);

	if (WaitingRoomWidget)
	{
		WaitingRoomWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (APlayerController* LocalPC = World->GetFirstPlayerController())
	{
		LocalPC->SetInputMode(FInputModeUIOnly());
		LocalPC->bShowMouseCursor = false;
		LocalPC->bEnableClickEvents = false;
	}

	World->GetTimerManager().ClearTimer(StartGameLoadingTransitionTimeoutTimerHandle);
	World->GetTimerManager().SetTimer(
		StartGameLoadingTransitionTimeoutTimerHandle,
		this,
		&UA302GameInstance::HandleStartGameLoadingTransitionTimeout,
		FMath::Max(TimeoutSeconds, 0.1f),
		false
	);

	UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] Start-game loading transition armed."));
}

void UA302GameInstance::CancelStartGameLoadingTransition()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		bStartGameLoadingTransitionActive = false;
		return;
	}

	World->GetTimerManager().ClearTimer(StartGameLoadingTransitionTimeoutTimerHandle);
	if (!bStartGameLoadingTransitionActive)
	{
		return;
	}

	bStartGameLoadingTransitionActive = false;
	HideLoadingWidget();

	if (A302RuntimeGuards::IsLobbyWorld(World) && WaitingRoomWidget)
	{
		WaitingRoomWidget->SetVisibility(ESlateVisibility::Visible);
	}

	if (APlayerController* LocalPC = World->GetFirstPlayerController())
	{
		LocalPC->SetInputMode(FInputModeUIOnly());
		LocalPC->bShowMouseCursor = true;
		LocalPC->bEnableClickEvents = true;
	}

	UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] Start-game loading transition cancelled."));
}

void UA302GameInstance::HandleStartGameLoadingTransitionTimeout()
{
	if (!bStartGameLoadingTransitionActive)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameInstance] Start-game loading transition timed out before map travel."));
	CancelStartGameLoadingTransition();
}


void UA302GameInstance::OnMessageReceived(const FString& Message)
{
	if (MessageRouter)
	{
		if (UFunction* HandleFunc = MessageRouter->FindFunction(TEXT("HandleMessage")))
		{
			MessageRouter->ProcessEvent(HandleFunc, const_cast<FString*>(&Message));
		}
	}
}

bool UA302GameInstance::IsNicknameValid(const FString& Nickname, FString& OutErrorMsg) const
{
	if (Nickname.IsEmpty())
	{
		OutErrorMsg = TEXT("닉네임을 입력해주세요.");
		return false;
	}

	if (Nickname.Len() > MaxNicknameLength)
	{
		OutErrorMsg = FString::Printf(TEXT("닉네임은 %d자 이내로 입력해주세요."), MaxNicknameLength);
		return false;
	}

	OutErrorMsg = TEXT("");
	return true;
}

void UA302GameInstance::ShowWaitingRoom(const FString& RoomCode)
{
	UWorld* World = GetWorld();
	if (!World || A302RuntimeGuards::IsDedicatedServerWorld(World))
		return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (!PC || WaitingRoomWidgetClass.IsNull())
		return;

	UClass* LoadedClass = WaitingRoomWidgetClass.LoadSynchronous();
	if (!LoadedClass) return;

	WaitingRoomWidget = CreateWidget<UUserWidget>(PC, LoadedClass);
	if (WaitingRoomWidget)
	{
		if (UFunction* SetRoomCodeFunc = WaitingRoomWidget->FindFunction(TEXT("SetRoomCode")))
		{
			WaitingRoomWidget->ProcessEvent(SetRoomCodeFunc, const_cast<FString*>(&RoomCode));
		}
		WaitingRoomWidget->AddToViewport();

		if (LobbyWidget)
		{
			LobbyWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

FString UA302GameInstance::ResolveRoomCodeForInGameWorld(UWorld* LoadedWorld) const
{
	FString ResolvedRoomCode = CurrentRoomCode;
	if (ResolvedRoomCode.IsEmpty() && LoadedWorld)
	{
		ResolvedRoomCode = FString(LoadedWorld->URL.GetOption(TEXT("roomCode="), TEXT("")));
		if (ResolvedRoomCode.IsEmpty())
		{
			ResolvedRoomCode = FString(LoadedWorld->URL.GetOption(TEXT("RoomCode="), TEXT("")));
		}
	}
	return ResolvedRoomCode;
}

void UA302GameInstance::EnsureLocalRoomLevelInstance(UWorld* LoadedWorld)
{
	if (!LoadedWorld || A302RuntimeGuards::IsDedicatedServerWorld(LoadedWorld))
		return;

	const FString ResolvedRoomCode = ResolveRoomCodeForInGameWorld(LoadedWorld);
	if (ResolvedRoomCode.IsEmpty())
		return;

	if (CurrentRoomCode.IsEmpty())
	{
		CurrentRoomCode = ResolvedRoomCode;
	}

	if (LocalRoomStreamingLevel && LocalRoomStreamingRoomCode == ResolvedRoomCode)
		return;

	// Use hardcoded path or resolve via helper
	constexpr TCHAR TemplateLevelPath[] = TEXT("/Game/PersonalWorkSpace/wjtmd28/MyMap");
	const FVector RoomOffset = A302RoomWorldOffset::ResolveRoomOffset(ResolvedRoomCode);
	const FString LevelInstanceNameOverride = A302RoomWorldOffset::BuildLevelInstanceNameOverride(ResolvedRoomCode);

	bool bLoadRequested = false;
	ULevelStreamingDynamic* StreamingLevel = ULevelStreamingDynamic::LoadLevelInstance(
		LoadedWorld,
		TemplateLevelPath,
		RoomOffset,
		FRotator::ZeroRotator,
		bLoadRequested,
		LevelInstanceNameOverride
	);

	if (bLoadRequested && StreamingLevel)
	{
		StreamingLevel->SetShouldBeLoaded(true);
		StreamingLevel->SetShouldBeVisible(true);
		LocalRoomStreamingLevel = StreamingLevel;
		LocalRoomStreamingRoomCode = ResolvedRoomCode;
		UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] Requested local room streaming level. room=%s map=%s"), *ResolvedRoomCode, TemplateLevelPath);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameInstance] Failed to request local room streaming level. room=%s map=%s"), *ResolvedRoomCode, TemplateLevelPath);
	}
}
