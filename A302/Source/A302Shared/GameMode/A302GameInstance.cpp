// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/A302GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "A302RuntimeGuards.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Room/RoomWorldOffset.h"
#include "Blueprint/UserWidget.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

UA302GameInstance::UA302GameInstance()
{
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

void UA302GameInstance::OnMapLoaded(UWorld* LoadedWorld)
{
	if (!LoadedWorld || LoadedWorld != GetWorld() || A302RuntimeGuards::IsDedicatedServerWorld(LoadedWorld))
		return;

	FString MapName = LoadedWorld->GetMapName();
	UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] Map Loaded: %s"), *MapName);

	LoadedWorld->GetTimerManager().ClearTimer(LobbyWidgetRetryTimerHandle);

	// Remove Lobby UI when entering in-game
	if (A302RuntimeGuards::IsInGameWorld(LoadedWorld))
	{
		EnsureLocalRoomLevelInstance(LoadedWorld);

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
	}
}
