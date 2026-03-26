#include "Voice/PrivateVoiceChatComponent.h"
#include "Voice/Profiling/VoiceProfiler.h"
#include "Voice/Capture/VoiceCaptureProcessor.h"
#include "Voice/Playback/VoiceAudioReceiver.h"
#include "Voice/Network/VoiceNetworkClient.h"
#include "Voice/Codec/VoiceCodec.h"
#include "Voice/Strategy/DistanceVoiceChatStrategy.h"
#include "Voice/Strategy/LobbyVoiceChatStrategy.h"
#include "Voice/Strategy/VoiceChatStrategyBase.h"
#include "GameMode/A302GameInstance.h"
#include "Network/GameNetworkSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameMode/A302PlayerState.h"
#include "EngineUtils.h"
#include "A302RuntimeGuards.h"

UPrivateVoiceChatComponent::UPrivateVoiceChatComponent()
{
    PrimaryComponentTick.bCanEverTick = false; 
    PrimaryComponentTick.bStartWithTickEnabled = false; 
}

void UPrivateVoiceChatComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!A302RuntimeGuards::ShouldRunClientOnlyLogic(this))
    {
        UE_LOG(LogVoiceChat, Log, TEXT("[Voice] Dedicated Server detected. Skipping local voice capture/playback initialization."));
        return;
    }

    APawn* OwnerPawn = Cast<APawn>(GetOwner());
    const bool bIsLocalPawn = OwnerPawn && OwnerPawn->IsLocallyControlled();
    bIsInitializedAsLocal = bIsLocalPawn;

    if (const AA302PlayerState* A302PlayerState = OwnerPawn ? OwnerPawn->GetPlayerState<AA302PlayerState>() : nullptr)
    {
        roomCode = A302PlayerState->GetRoomCode();
    }

    AudioReceiver = NewObject<UVoiceAudioReceiver>(this);
    if (AudioReceiver)
    {
        AudioReceiver->Initialize(this);
    }

    if (!bIsLocalPawn)
    {
        UE_LOG(LogVoiceChat, Verbose, TEXT("[Voice] Remote pawn component initialized as playback-only. owner=%s"), *GetNameSafe(GetOwner()));
        return;
    }

    // 중앙 설정(GameNetworkSubsystem)에서 서버 주소 가져오기 (클라이언트에서만 유효)
    if (GetNetMode() == NM_Client || GetNetMode() == NM_Standalone)
    {
        if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
        {
            if (UGameNetworkSubsystem* NetworkSubsystem = Cast<UGameNetworkSubsystem>(GI->GetSubsystemBase(UGameNetworkSubsystem::StaticClass())))
            {
                VoiceServerUrl = NetworkSubsystem->GetVoiceURL();
            }
        }
    }

    UE_LOG(LogVoiceChat, Log, TEXT("[Voice] BeginPlay - Facade Initialize"));

    // 모듈 생성 및 초기화
    CaptureProcessor = NewObject<UVoiceCaptureProcessor>(this);
    if (CaptureProcessor)
    {
        CaptureProcessor->Initialize();
        CaptureProcessor->OnVoiceDataCaptured.BindUObject(this, &UPrivateVoiceChatComponent::OnVoiceDataCaptured);
    }

    NetworkClient = NewObject<UVoiceNetworkClient>(this);
    if (NetworkClient)
    {
        NetworkClient->Initialize(this);
        NetworkClient->OnBinaryPacketReceived.BindUObject(this, &UPrivateVoiceChatComponent::OnNetworkBinaryMessageReceived);
    }

    // 개별 화자용 디코더는 패킷 수신 시(OnNetworkBinaryMessageReceived) 동적으로 생성하여 맵(SpeakerCodecs)에 저장합니다.

    if (!LobbyStrategy)
        LobbyStrategy = NewObject<ULobbyVoiceChatStrategy>(this);

    if (!DistanceStrategy)
        DistanceStrategy = NewObject<UDistanceVoiceChatStrategy>(this);

    const float EffectiveHearingDistance =
        FMath::IsNearlyEqual(DefaultInGameHearingDistance, 1800.f) ? 4200.f : FMath::Max(100.f, DefaultInGameHearingDistance);

    if (DistanceStrategy)
        DistanceStrategy->SetHearingDistance(EffectiveHearingDistance);

    // 현재 월드가 로비인지 인게임인지에 따라 보이스 모드를 결정
    // Lobby → 로비 모드 (전체 통화)
    // InGame → 거리 기반 모드
    if (UWorld* World = GetWorld())
    {
        if (A302RuntimeGuards::IsLobbyWorld(World))
        {
            SetLobbyMode();
            UE_LOG(LogVoiceChat, Log, TEXT("[Voice] GameMode=Lobby → 로비 보이스 모드 적용"));
        }
        else
        {
            SetDistanceMode(EffectiveHearingDistance);
            UE_LOG(LogVoiceChat, Log, TEXT("[Voice] GameMode=InGame → 거리 보이스 모드 적용 (%.0f)"), EffectiveHearingDistance);
        }
    }

    if (bAutoConnectOnBeginPlay)
    {
        if (OwnerPawn)
        {
            if (UA302GameInstance* GI = Cast<UA302GameInstance>(UGameplayStatics::GetGameInstance(this)))
            {
                SetRoomCode(GI->CurrentRoomCode);
            }
            ConnectToVoiceServer();
        }
    }
}

void UPrivateVoiceChatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    DisconnectFromVoiceServer();
    
    if (CaptureProcessor)
    {
        CaptureProcessor->Stop(GetWorld());
    }

    Super::EndPlay(EndPlayReason);
}

// ===== 마이크 제어 =====

void UPrivateVoiceChatComponent::StartMicrophone()
{
    if (CaptureProcessor) CaptureProcessor->Start(GetWorld());
}

void UPrivateVoiceChatComponent::StopMicrophone()
{
    if (CaptureProcessor) CaptureProcessor->Stop(GetWorld());
}

void UPrivateVoiceChatComponent::ToggleMicrophone()
{
    if (CaptureProcessor) CaptureProcessor->ToggleMicrophone(GetWorld());
}

bool UPrivateVoiceChatComponent::IsMicrophoneActive() const
{
    return CaptureProcessor ? CaptureProcessor->IsMicrophoneActive() : false;
}

bool UPrivateVoiceChatComponent::ChangeMicrophone(const FString& DeviceName)
{
    return CaptureProcessor ? CaptureProcessor->ChangeMicrophone(DeviceName) : false;
}

// ===== 서버 연결 =====

void UPrivateVoiceChatComponent::ConnectToVoiceServer()
{
    if (NetworkClient && !VoiceServerUrl.IsEmpty())
    {
        if (roomCode.IsEmpty())
        {
            const FString ResolvedRoomCode = ResolveCurrentRoomCode();
            if (!ResolvedRoomCode.IsEmpty())
            {
                SetRoomCode(ResolvedRoomCode);
            }
        }

        NetworkClient->Connect(VoiceServerUrl);

        if (!roomCode.IsEmpty())
        {
            // 서버 connectedClients에 즉시 등록 (마이크를 켜지 않아도 수신 가능하도록)
            TArray<uint8> EmptyPayload;
            NetworkClient->SendVoiceData(EmptyPayload, roomCode, TEXT("join_voice"), GetMyPlayerName());
            UE_LOG(LogVoiceChat, Log, TEXT("[Voice] ConnectToVoiceServer: join_voice 초기 가입 패킷 전송 완료. room=%s"), *roomCode);
        }
        else
        {
            UE_LOG(LogVoiceChat, Warning, TEXT("[Voice] ConnectToVoiceServer: roomCode is empty. join_voice packet skipped."));
        }
    }
}

void UPrivateVoiceChatComponent::DisconnectFromVoiceServer()
{
    // BeginPlay에서 로컬로 판정되었거나, 현재 로컬 컨트롤 중인 경우에만 종료 패킷 전송
    bool bShouldDisconnect = bIsInitializedAsLocal;
    
    if (!bShouldDisconnect)
    {
        if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
        {
            bShouldDisconnect = OwnerPawn->IsLocallyControlled();
        }
    }

    if (!bShouldDisconnect)
    {
        return; 
    }

    if (NetworkClient) 
    {
        UE_LOG(LogVoiceChat, Log, TEXT("[Voice] DisconnectFromVoiceServer - Sending leave packet for Room: %s, User: %s"), *roomCode, *GetMyPlayerName());
        NetworkClient->Disconnect(roomCode, GetMyPlayerName());
    }
}

// ===== 전략(Strategy) 관리 =====

void UPrivateVoiceChatComponent::SetLobbyMode()
{
    ApplyStrategy(LobbyStrategy);
}

void UPrivateVoiceChatComponent::SetDistanceMode(float NewDistance)
{
    if (DistanceStrategy && NewDistance > 0.f)
    {
        DistanceStrategy->SetHearingDistance(NewDistance);
    }
    ApplyStrategy(DistanceStrategy);
}

void UPrivateVoiceChatComponent::SetRoomCode(const FString& InRoomCode)
{
    UE_LOG(LogVoiceChat, Verbose, TEXT("[Voice] SetRoomCode: Old='%s' New='%s'"), *roomCode, *InRoomCode);
    roomCode = InRoomCode;
}

EVoiceChatMode UPrivateVoiceChatComponent::GetCurrentMode() const
{
    return ActiveStrategy ? ActiveStrategy->GetMode() : EVoiceChatMode::Lobby;
}

bool UPrivateVoiceChatComponent::CanHearActor(const AActor* SpeakerActor) const
{
    if (!SpeakerActor || !ActiveStrategy) return false;

    const UPrivateVoiceChatComponent* SpeakerComp = SpeakerActor->FindComponentByClass<UPrivateVoiceChatComponent>();
    if (!SpeakerComp) return false;

    return ActiveStrategy->CanReceiveVoice(this, SpeakerComp);
}

void UPrivateVoiceChatComponent::ApplyStrategy(UVoiceChatStrategyBase* NewStrategy)
{
    if (!NewStrategy) return;
    ActiveStrategy = NewStrategy;
    OnVoiceModeChanged.Broadcast(ActiveStrategy->GetMode());
}

// ===== 헬퍼 =====

FString UPrivateVoiceChatComponent::GetMyPlayerName() const
{
    // 1순위: PlayerState에서 가져오기
    if (APawn* MyPawn = Cast<APawn>(GetOwner()))
    {
        if (IsValid(MyPawn))
        {
            if (APlayerState* PS = MyPawn->GetPlayerState())
            {
                if (IsValid(PS))
                {
                    return PS->GetPlayerName();
                }
            }
        }
    }

    // 2순위: GameInstance에서 가져오기
    if (UA302GameInstance* GI = Cast<UA302GameInstance>(UGameplayStatics::GetGameInstance(this)))
    {
        if (!GI->MyPlayerName.IsEmpty())
        {
            return GI->MyPlayerName;
        }
    }

    // Fallback
    return GetNameSafe(GetOwner());
}

FString UPrivateVoiceChatComponent::ResolveCurrentRoomCode() const
{
    if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
    {
        if (const AA302PlayerState* A302PlayerState = OwnerPawn->GetPlayerState<AA302PlayerState>())
        {
            const FString FromPlayerState = A302PlayerState->GetRoomCode();
            if (!FromPlayerState.IsEmpty())
            {
                return FromPlayerState;
            }
        }
    }

    if (const UA302GameInstance* GI = Cast<UA302GameInstance>(UGameplayStatics::GetGameInstance(this)))
    {
        if (!GI->CurrentRoomCode.IsEmpty())
        {
            return GI->CurrentRoomCode;
        }
    }

    if (const UWorld* World = GetWorld())
    {
        FString FromUrl = FString(World->URL.GetOption(TEXT("roomCode="), TEXT("")));
        if (FromUrl.IsEmpty())
        {
            FromUrl = FString(World->URL.GetOption(TEXT("RoomCode="), TEXT("")));
        }
        return FromUrl;
    }

    return FString();
}

// ===== 송신 콜백 =====

void UPrivateVoiceChatComponent::OnVoiceDataCaptured(const TArray<uint8>& VoiceData)
{
    if (!NetworkClient || !ActiveStrategy) return;

    if (roomCode.IsEmpty())
    {
        const FString ResolvedRoomCode = ResolveCurrentRoomCode();
        if (!ResolvedRoomCode.IsEmpty())
        {
            SetRoomCode(ResolvedRoomCode);
        }
    }
    if (roomCode.IsEmpty())
    {
        return;
    }

    const FString ModeString = (GetCurrentMode() == EVoiceChatMode::Lobby) ? TEXT("lobby") : TEXT("distance");

    NetworkClient->SendVoiceData(VoiceData, roomCode, ModeString, GetMyPlayerName());
}

// ===== 수신 콜백 =====

void UPrivateVoiceChatComponent::OnNetworkBinaryMessageReceived(const FString& InRoomCode, const FString& SpeakerName, const TArray<uint8>& VoiceData)
{
    if (roomCode.IsEmpty() && !InRoomCode.IsEmpty())
    {
        SetRoomCode(InRoomCode);
    }

    // 다른 방 음성은 즉시 무시
    if (roomCode.IsEmpty() || InRoomCode.IsEmpty() || InRoomCode != roomCode)
    {
        return;
    }

    // 본인 목소리 메아리 방지
    if (SpeakerName == GetMyPlayerName()) return; 

    // 발화자 전용 독립 코덱 탐색 및 생성 (GameThread에서 수행하므로 맵 접근 안전성 보장)
    UVoiceCodec* SpeakerCodec = nullptr;
    if (TObjectPtr<UVoiceCodec>* FoundCodec = SpeakerCodecs.Find(SpeakerName))
    {
        SpeakerCodec = *FoundCodec;
    }
    else
    {
        SpeakerCodec = NewObject<UVoiceCodec>(this);
        if (SpeakerCodec)
        {
            SpeakerCodec->Init();
            SpeakerCodecs.Add(SpeakerName, SpeakerCodec);
        }
    }

    // 상대방 오디오 리시버(스피커 객체) 확보 (액터 검색은 GameThread에서 로직 수행 필수)
    bool bSpeakerFound = false;
    UVoiceAudioReceiver* TargetReceiver = nullptr;

    if (UWorld* World = GetWorld())
    {
        for (TActorIterator<APawn> It(World); It; ++It)
        {
            APawn* CurrentPawn = *It;
            if (!IsValid(CurrentPawn)) continue;

            APlayerState* PS = CurrentPawn->GetPlayerState();
            if (!IsValid(PS) || PS->GetPlayerName() != SpeakerName) continue;

            // 발화자 발견!
            bSpeakerFound = true;
            UPrivateVoiceChatComponent* OtherComp = CurrentPawn->FindComponentByClass<UPrivateVoiceChatComponent>();
            if (!OtherComp)
            {
                OtherComp = NewObject<UPrivateVoiceChatComponent>(CurrentPawn, UPrivateVoiceChatComponent::StaticClass(), TEXT("PrivateVoiceChatComponent"));
                if (OtherComp)
                {
                    CurrentPawn->AddInstanceComponent(OtherComp);
                    OtherComp->RegisterComponent();
                    UE_LOG(LogVoiceChat, Log, TEXT("[Voice] Added runtime voice component to speaker pawn: %s"), *GetNameSafe(CurrentPawn));
                }
            }

            if (OtherComp)
            {
                if (OtherComp->GetRoomCode().IsEmpty())
                {
                    OtherComp->SetRoomCode(InRoomCode);
                }

                if (CanHearActor(CurrentPawn))
                {
                    TargetReceiver = OtherComp->AudioReceiver;
                }
            }
            break;
        }
    }

    // 발화자 미확인/전략 차단 시 재생 금지
    if (!bSpeakerFound || !TargetReceiver || !SpeakerCodec) return;

    // 동일 화자 코덱을 여러 스레드에서 동시에 디코드하면 Opus 상태가 꼬일 수 있으므로,
    // 수신 순서대로 단일 스레드(GameThread)에서 디코드/재생합니다.
    TArray<uint8> PCMData;
    if (!SpeakerCodec->Decode(VoiceData, PCMData) || PCMData.Num() == 0)
    {
        return; // 디코딩 실패 → 깨진 패킷 드롭
    }

    TargetReceiver->PlayVoice(PCMData);
}
