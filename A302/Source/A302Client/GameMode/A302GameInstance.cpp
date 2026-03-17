// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/A302GameInstance.h"
#include "GameMode/LobbyMessageRouter.h"
#include "Network/GameNetworkSubsystem.h"
#include "UI/LobbyWidget.h"
#include "UI/WaitingRoomWidget.h"
#include "Kismet/GameplayStatics.h"
#include "A302RuntimeGuards.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Room/RoomWorldOffset.h"

void UA302GameInstance::Init()
{
    Super::Init();

    if (A302RuntimeGuards::IsDedicatedServerWorld(this))
    {
        UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] Dedicated Server instance detected. Skipping lobby UI/WebSocket initialization."));
        return;
    }

    MessageRouter = NewObject<ULobbyMessageRouter>(this);
    if (MessageRouter)
    {
        MessageRouter->Initialize(this);
    }

    GameNetworkSubsystem = Cast<UGameNetworkSubsystem>(GetSubsystemBase(UGameNetworkSubsystem::StaticClass()));
    if (GameNetworkSubsystem)
    {
        GameNetworkSubsystem->Connect(EProtocolType::WebSocket, GameNetworkSubsystem->GetLobbyURL());
        GameNetworkSubsystem->OnPacketReceived.AddDynamic(this, &UA302GameInstance::OnMessageReceived);
    }
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UA302GameInstance::OnMapLoaded);
}

void UA302GameInstance::OnStart()
{
    Super::OnStart();

    if (A302RuntimeGuards::IsDedicatedServerWorld(this))
        return;

    UWorld *CurrentWorld = GetWorld();
    if (!CurrentWorld)
        return;

    OnMapLoaded(CurrentWorld);
}

void UA302GameInstance::OnMapLoaded(UWorld *LoadedWorld)
{
    if (!LoadedWorld)
        return;
    if (LoadedWorld->GetNetMode() == NM_DedicatedServer)
        return;
    if (LoadedWorld != GetWorld())
        return;

    FString MapName = LoadedWorld->GetMapName();
    UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] 맵 로드됨: %s"), *MapName);

    LoadedWorld->GetTimerManager().ClearTimer(LobbyWidgetRetryTimerHandle);

    // 인게임 레벨로 이동 시 로비 UI 제거
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

    // 로비 레벨에서만 로비 위젯 생성
    if (!A302RuntimeGuards::IsLobbyWorld(LoadedWorld))
        return;

    if (TryCreateLobbyWidget(LoadedWorld))
        return;

    LoadedWorld->GetTimerManager().SetTimer(
        LobbyWidgetRetryTimerHandle,
        FTimerDelegate::CreateLambda([this, LoadedWorld]()
                                     {
                                         TryCreateLobbyWidget(LoadedWorld);
                                     }),
        0.1f,
        true
    );
}

bool UA302GameInstance::TryCreateLobbyWidget(UWorld *LoadedWorld)
{
    if (!LoadedWorld || LoadedWorld != GetWorld())
        return false;

    if (LoadedWorld->GetNetMode() == NM_DedicatedServer)
        return false;

    if (!LobbyWidgetClass)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode/A302GameInstance] LobbyWidgetClass가 NULL입니다!"));
        return false;
    }

    APlayerController *PC = LoadedWorld->GetFirstPlayerController();
    if (!PC)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[GameMode/A302GameInstance] LobbyWidget 생성 대기 중: PlayerController가 아직 없습니다."));
        return false;
    }

    if (LobbyWidget)
    {
        LobbyWidget->SetVisibility(ESlateVisibility::Visible);
        LoadedWorld->GetTimerManager().ClearTimer(LobbyWidgetRetryTimerHandle);
        return true;
    }

    PC->bShowMouseCursor = true;
    PC->bEnableClickEvents = true;
    PC->SetInputMode(FInputModeUIOnly());

    LobbyWidget = CreateWidget<ULobbyWidget>(PC, LobbyWidgetClass);
    if (!LobbyWidget)
    {
        UE_LOG(LogTemp, Error, TEXT("[GameMode/A302GameInstance] LobbyWidget 생성 실패"));
        return false;
    }

    LobbyWidget->AddToViewport();
    LoadedWorld->GetTimerManager().ClearTimer(LobbyWidgetRetryTimerHandle);
    UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] LobbyWidget 생성 성공"));
    return true;
}

void UA302GameInstance::ConnectToServer(const FString &URL)
{
    if (GameNetworkSubsystem)
    {
        FString TargetURL = GameNetworkSubsystem->GetLobbyURL();
        UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameInstance] Blueprint requested connect to %s, but overriding with code endpoint: %s"), *URL, *TargetURL);
        GameNetworkSubsystem->Connect(EProtocolType::WebSocket, TargetURL);
    }
}

void UA302GameInstance::SendToServer(const FString &Message)
{
    if (GameNetworkSubsystem)
    {
        GameNetworkSubsystem->SendPacket(EProtocolType::WebSocket, Message);
    }
}

void UA302GameInstance::OnMessageReceived(const FString &Message)
{
    if (MessageRouter)
    {
        MessageRouter->HandleMessage(Message);
    }
}

void UA302GameInstance::ShowWaitingRoom(const FString &RoomCode)
{
    UWorld *World = GetWorld();
    if (!World)
        return;

    APlayerController *PC = UGameplayStatics::GetPlayerController(World, 0);
    if (!PC)
        return;

    if (WaitingRoomWidgetClass)
    {
        WaitingRoomWidget = CreateWidget<UWaitingRoomWidget>(PC, WaitingRoomWidgetClass);
        if (WaitingRoomWidget)
        {
            WaitingRoomWidget->SetRoomCode(RoomCode);
            WaitingRoomWidget->AddToViewport();
            WaitingRoomWidget->OnPlayerEntered(MyPlayerName);

            if (LobbyWidget)
            {
                LobbyWidget->SetVisibility(ESlateVisibility::Collapsed);
            }

            UE_LOG(LogTemp, Log, TEXT("[GameMode/A302GameInstance] WaitingRoomWidget 생성 성공"));
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

    return A302RoomWorldOffset::NormalizeRoomCode(ResolvedRoomCode);
}

void UA302GameInstance::EnsureLocalRoomLevelInstance(UWorld* LoadedWorld)
{
    if (!LoadedWorld)
    {
        return;
    }

    const FString ResolvedRoomCode = ResolveRoomCodeForInGameWorld(LoadedWorld);
    if (ResolvedRoomCode.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[GameMode/A302GameInstance] In-game map loaded but roomCode is empty. Skip local room instance loading."));
        return;
    }

    if (CurrentRoomCode.IsEmpty())
    {
        CurrentRoomCode = ResolvedRoomCode;
    }

    if (LocalRoomStreamingLevel && LocalRoomStreamingRoomCode == ResolvedRoomCode)
    {
        return;
    }

    constexpr TCHAR TemplateLevelPath[] = TEXT("/Game/PersonalWorkSpace/siris/sirisMap");
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

    if (!bLoadRequested || !StreamingLevel)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[GameMode/A302GameInstance] Failed to load local room instance. room=%s map=%s instance=%s offset=(%.0f, %.0f, %.0f)"),
            *ResolvedRoomCode,
            TemplateLevelPath,
            *LevelInstanceNameOverride,
            RoomOffset.X,
            RoomOffset.Y,
            RoomOffset.Z
        );
        return;
    }

    StreamingLevel->SetShouldBeLoaded(true);
    StreamingLevel->SetShouldBeVisible(true);
    LocalRoomStreamingLevel = StreamingLevel;
    LocalRoomStreamingRoomCode = ResolvedRoomCode;

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[GameMode/A302GameInstance] Local room instance requested. room=%s map=%s instance=%s offset=(%.0f, %.0f, %.0f)"),
        *ResolvedRoomCode,
        TemplateLevelPath,
        *LevelInstanceNameOverride,
        RoomOffset.X,
        RoomOffset.Y,
        RoomOffset.Z
    );
}
