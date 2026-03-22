// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/A302GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "A302RuntimeGuards.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/World.h"
#include "Room/RoomWorldOffset.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ProgressBar.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	enum class EFallbackLobbyPendingAction : int32
	{
		None = 0,
		CreateRoom,
		EnterRoom,
		FindRoom
	};

	constexpr TCHAR WorkspaceLobby2ClassPath[] = TEXT("/Game/WorkSpace/UI/WBP_Lobby2.WBP_Lobby2_C");
	constexpr TCHAR PersonalLobby2ClassPath[] = TEXT("/Game/PersonalWorkSpace/wjtmd28/WBP_Lobby2.WBP_Lobby2_C");
	constexpr TCHAR RoomListPopupClassPath[] = TEXT("/Game/PersonalWorkSpace/sikk806/WBP_RoomListPopup.WBP_RoomListPopup_C");
	constexpr TCHAR LoadingWidgetClassPath[] = TEXT("/Game/WorkSpace/UI/WBP_Loading.WBP_Loading_C");
	constexpr TCHAR TestLevelMapName[] = TEXT("testLevel");
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

	FString GetNormalizedMapName(const UWorld* World)
	{
		if (!World)
		{
			return FString();
		}

		FString MapName = World->GetMapName();
		if (MapName.StartsWith(TEXT("UEDPIE_")))
		{
			const int32 PrefixLen = 7;
			const int32 UnderscoreIndex = MapName.Find(TEXT("_"), ESearchCase::IgnoreCase, ESearchDir::FromStart, PrefixLen);
			if (UnderscoreIndex != INDEX_NONE)
			{
				MapName = MapName.Mid(UnderscoreIndex + 1);
			}
		}

		return FPackageName::GetShortName(MapName);
	}
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

	TSubclassOf<UUserWidget> LoadedClass = ResolveLobbyWidgetClass(LoadedWorld);
	if (!LoadedClass)
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

	LobbyWidget = CreateWidget<UUserWidget>(PC, LoadedClass);
	if (LobbyWidget)
	{
		LobbyWidget->AddToViewport();
		SetupFallbackLobbyBindings();
		LoadedWorld->GetTimerManager().ClearTimer(LobbyWidgetRetryTimerHandle);
		UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] LobbyWidget created successfully."));
		return true;
	}

	return false;
}

void UA302GameInstance::SetupFallbackLobbyBindings()
{
	bFallbackLobbyBindingsActive = false;
	FallbackLobbyPendingAction = static_cast<int32>(EFallbackLobbyPendingAction::None);

	if (!LobbyWidget || !LobbyWidget->WidgetTree)
	{
		return;
	}

	UButton* CreateRoomButton = FindLobbyButton(TEXT("Btn_CreateRoom"));
	UButton* EnterRoomButton = FindLobbyButton(TEXT("Btn_EnterRoom"));
	UButton* FindRoomButton = FindLobbyButton(TEXT("Btn_FindRoom"));
	UButton* ExitButton = FindLobbyButton(TEXT("Btn_Exit"));

	if (CreateRoomButton && !CreateRoomButton->OnClicked.IsBound())
	{
		CreateRoomButton->OnClicked.AddDynamic(this, &UA302GameInstance::HandleFallbackLobbyCreateRoomClicked);
		bFallbackLobbyBindingsActive = true;
	}
	if (EnterRoomButton && !EnterRoomButton->OnClicked.IsBound())
	{
		EnterRoomButton->OnClicked.AddDynamic(this, &UA302GameInstance::HandleFallbackLobbyEnterRoomClicked);
		bFallbackLobbyBindingsActive = true;
	}
	if (FindRoomButton && !FindRoomButton->OnClicked.IsBound())
	{
		FindRoomButton->OnClicked.AddDynamic(this, &UA302GameInstance::HandleFallbackLobbyFindRoomClicked);
		bFallbackLobbyBindingsActive = true;
	}
	if (ExitButton && !ExitButton->OnClicked.IsBound())
	{
		ExitButton->OnClicked.AddDynamic(this, &UA302GameInstance::HandleFallbackLobbyExitClicked);
		bFallbackLobbyBindingsActive = true;
	}

	if (bFallbackLobbyBindingsActive)
	{
		OnRoomCreated.RemoveDynamic(this, &UA302GameInstance::HandleFallbackLobbyRoomCreated);
		OnNicknameAvailable.RemoveDynamic(this, &UA302GameInstance::HandleFallbackLobbyNicknameAvailable);
		OnRoomCreated.AddDynamic(this, &UA302GameInstance::HandleFallbackLobbyRoomCreated);
		OnNicknameAvailable.AddDynamic(this, &UA302GameInstance::HandleFallbackLobbyNicknameAvailable);
		UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] Fallback lobby bindings enabled for current lobby widget."));
	}
}

UButton* UA302GameInstance::FindLobbyButton(FName WidgetName) const
{
	return LobbyWidget && LobbyWidget->WidgetTree ? Cast<UButton>(LobbyWidget->WidgetTree->FindWidget(WidgetName)) : nullptr;
}

UEditableTextBox* UA302GameInstance::FindLobbyEditableTextBox(FName WidgetName) const
{
	return LobbyWidget && LobbyWidget->WidgetTree ? Cast<UEditableTextBox>(LobbyWidget->WidgetTree->FindWidget(WidgetName)) : nullptr;
}

bool UA302GameInstance::ValidateLobbyPlayerName(FString& OutPlayerName) const
{
	if (const UEditableTextBox* PlayerNameInput = FindLobbyEditableTextBox(TEXT("Input_PlayerName")))
	{
		OutPlayerName = PlayerNameInput->GetText().ToString().TrimStartAndEnd();
	}
	else
	{
		OutPlayerName.Reset();
	}

	FString ErrorMessage;
	if (!IsNicknameValid(OutPlayerName, ErrorMessage))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameInstance] %s"), *ErrorMessage);
		return false;
	}

	return true;
}

void UA302GameInstance::SendLobbyNicknameCheck(const FString& PlayerName)
{
	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("playerName"), PlayerName);

	TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
	Json->SetStringField(TEXT("type"), TEXT("check_nickname"));
	Json->SetObjectField(TEXT("data"), Data);

	FString Output;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

	SendToServer(Output);
	UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] Fallback lobby nickname check sent: %s"), *PlayerName);
}

void UA302GameInstance::SendCreateRoomRequest(const FString& PlayerName)
{
	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("playerName"), PlayerName);

	TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
	Json->SetStringField(TEXT("type"), TEXT("create_room"));
	Json->SetObjectField(TEXT("data"), Data);

	FString Output;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

	SendToServer(Output);
	UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] Fallback lobby create room requested."));
}

void UA302GameInstance::SendJoinRoomRequest(const FString& RoomCode, const FString& PlayerName)
{
	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("roomCode"), RoomCode);
	Data->SetStringField(TEXT("playerName"), PlayerName);

	TSharedPtr<FJsonObject> Json = MakeShareable(new FJsonObject);
	Json->SetStringField(TEXT("type"), TEXT("join_room"));
	Json->SetObjectField(TEXT("data"), Data);

	FString Output;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Output);
	FJsonSerializer::Serialize(Json.ToSharedRef(), Writer);

	SendToServer(Output);
	UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] Fallback lobby join room requested. code=%s"), *RoomCode);
}

void UA302GameInstance::OpenRoomListPopupFallback()
{
	if (!LobbyWidget)
	{
		return;
	}

	if (UClass* PopupClass = LoadClass<UUserWidget>(nullptr, RoomListPopupClassPath))
	{
		if (UWorld* World = GetWorld())
		{
			if (UUserWidget* Popup = CreateWidget<UUserWidget>(World, PopupClass))
			{
				Popup->AddToViewport();
				UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] Fallback room list popup opened."));
			}
		}
	}
}

void UA302GameInstance::HandleFallbackLobbyCreateRoomClicked()
{
	FString PlayerName;
	if (!ValidateLobbyPlayerName(PlayerName))
	{
		return;
	}

	MyPlayerName = PlayerName;
	FallbackLobbyPendingAction = static_cast<int32>(EFallbackLobbyPendingAction::CreateRoom);
	SendLobbyNicknameCheck(PlayerName);
}

void UA302GameInstance::HandleFallbackLobbyEnterRoomClicked()
{
	FString PlayerName;
	if (!ValidateLobbyPlayerName(PlayerName))
	{
		return;
	}

	const UEditableTextBox* RoomCodeInput = FindLobbyEditableTextBox(TEXT("Input_RoomCode"));
	const FString RoomCode = RoomCodeInput ? RoomCodeInput->GetText().ToString().TrimStartAndEnd() : FString();
	if (RoomCode.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameInstance] 방 코드를 입력해주세요."));
		return;
	}

	MyPlayerName = PlayerName;
	FallbackLobbyPendingAction = static_cast<int32>(EFallbackLobbyPendingAction::EnterRoom);
	SendLobbyNicknameCheck(PlayerName);
}

void UA302GameInstance::HandleFallbackLobbyFindRoomClicked()
{
	FString PlayerName;
	if (!ValidateLobbyPlayerName(PlayerName))
	{
		return;
	}

	MyPlayerName = PlayerName;
	FallbackLobbyPendingAction = static_cast<int32>(EFallbackLobbyPendingAction::FindRoom);
	SendLobbyNicknameCheck(PlayerName);
}

void UA302GameInstance::HandleFallbackLobbyExitClicked()
{
	UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] Fallback lobby exit clicked."));
	UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}

void UA302GameInstance::HandleFallbackLobbyRoomCreated(const FString& RoomCode)
{
	if (!bFallbackLobbyBindingsActive)
	{
		return;
	}

	if (UEditableTextBox* RoomCodeInput = FindLobbyEditableTextBox(TEXT("Input_RoomCode")))
	{
		RoomCodeInput->SetText(FText::FromString(RoomCode));
	}
}

void UA302GameInstance::HandleFallbackLobbyNicknameAvailable()
{
	if (!bFallbackLobbyBindingsActive)
	{
		return;
	}

	switch (static_cast<EFallbackLobbyPendingAction>(FallbackLobbyPendingAction))
	{
	case EFallbackLobbyPendingAction::CreateRoom:
		SendCreateRoomRequest(MyPlayerName);
		break;
	case EFallbackLobbyPendingAction::EnterRoom:
		if (const UEditableTextBox* RoomCodeInput = FindLobbyEditableTextBox(TEXT("Input_RoomCode")))
		{
			SendJoinRoomRequest(RoomCodeInput->GetText().ToString().TrimStartAndEnd(), MyPlayerName);
		}
		break;
	case EFallbackLobbyPendingAction::FindRoom:
		OpenRoomListPopupFallback();
		break;
	default:
		break;
	}

	FallbackLobbyPendingAction = static_cast<int32>(EFallbackLobbyPendingAction::None);
}

TSubclassOf<UUserWidget> UA302GameInstance::ResolveLobbyWidgetClass(UWorld* LoadedWorld) const
{
	const FString MapName = GetNormalizedMapName(LoadedWorld);
	if (MapName.Equals(TestLevelMapName, ESearchCase::IgnoreCase))
	{
		if (UClass* WorkspaceLobby2Class = LoadClass<UUserWidget>(nullptr, WorkspaceLobby2ClassPath))
		{
			return WorkspaceLobby2Class;
		}

		if (UClass* PersonalLobby2Class = LoadClass<UUserWidget>(nullptr, PersonalLobby2ClassPath))
		{
			return PersonalLobby2Class;
		}
	}

	if (LobbyWidgetClass.IsNull())
	{
		return nullptr;
	}

	return LobbyWidgetClass.LoadSynchronous();
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
