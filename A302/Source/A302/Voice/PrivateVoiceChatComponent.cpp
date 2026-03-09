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
#include "GameMode/LobbyGameMode.h"
#include "Network/GameNetworkSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "EngineUtils.h"

UPrivateVoiceChatComponent::UPrivateVoiceChatComponent()
{
    PrimaryComponentTick.bCanEverTick = false; 
    PrimaryComponentTick.bStartWithTickEnabled = false; 
}

void UPrivateVoiceChatComponent::BeginPlay()
{
    Super::BeginPlay();

    // 중앙 설정(GameNetworkSubsystem)에서 서버 주소 가져오기
    if (UGameInstance* GI = UGameplayStatics::GetGameInstance(this))
    {
        if (UGameNetworkSubsystem* NetworkSubsystem = GI->GetSubsystem<UGameNetworkSubsystem>())
        {
            VoiceServerUrl = NetworkSubsystem->GetVoiceURL();
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

    AudioReceiver = NewObject<UVoiceAudioReceiver>(this);
    if (AudioReceiver)
    {
        AudioReceiver->Initialize(this);
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

    if (DistanceStrategy)
        DistanceStrategy->SetHearingDistance(DefaultInGameHearingDistance);

    // GameMode에 따라 자동 보이스 모드 결정
    // LobbyGameMode → 로비 모드 (전체 통화)
    // InGameGameMode → 거리 기반 모드
    if (UWorld* World = GetWorld())
    {
        AGameModeBase* GM = World->GetAuthGameMode();
        if (GM && GM->IsA<ALobbyGameMode>())
        {
            SetLobbyMode();
            UE_LOG(LogVoiceChat, Log, TEXT("[Voice] GameMode=Lobby → 로비 보이스 모드 적용"));
        }
        else
        {
            SetDistanceMode(DefaultInGameHearingDistance);
            UE_LOG(LogVoiceChat, Log, TEXT("[Voice] GameMode=InGame → 거리 보이스 모드 적용 (%.0f)"), DefaultInGameHearingDistance);
        }
    }

    if (bAutoConnectOnBeginPlay)
    {
        if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
        {
            if (OwnerPawn->IsLocallyControlled())
            {
                ConnectToVoiceServer();
            }
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
        NetworkClient->Connect(VoiceServerUrl);
        
        // 서버 connectedClients에 즉시 등록 (마이크를 켜지 않아도 수신 가능하도록)
        TArray<uint8> EmptyPayload;
        NetworkClient->SendVoiceData(EmptyPayload, roomCode, TEXT("join_voice"), GetMyPlayerName());
        UE_LOG(LogVoiceChat, Log, TEXT("[Voice] ConnectToVoiceServer: join_voice 초기 가입 패킷 전송 완료."));
    }
}

void UPrivateVoiceChatComponent::DisconnectFromVoiceServer()
{
    if (NetworkClient) NetworkClient->Disconnect();
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
    UE_LOG(LogVoiceChat, Warning, TEXT("[Voice] SetRoomCode Called: Old='%s' New='%s'"), *roomCode, *InRoomCode);
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

// ===== 송신 콜백 =====

void UPrivateVoiceChatComponent::OnVoiceDataCaptured(const TArray<uint8>& VoiceData)
{
    if (!NetworkClient || !ActiveStrategy) return;

    const FString ModeString = (GetCurrentMode() == EVoiceChatMode::Lobby) ? TEXT("lobby") : TEXT("distance");

    NetworkClient->SendVoiceData(VoiceData, roomCode, ModeString, GetMyPlayerName());
}

// ===== 수신 콜백 =====

void UPrivateVoiceChatComponent::OnNetworkBinaryMessageReceived(const FString& InRoomCode, const FString& SpeakerName, const TArray<uint8>& VoiceData)
{
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
    bool bPlayed = false;
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
            if (UPrivateVoiceChatComponent* OtherComp = CurrentPawn->FindComponentByClass<UPrivateVoiceChatComponent>())
            {
                if (CanHearActor(CurrentPawn))
                {
                    TargetReceiver = OtherComp->AudioReceiver;
                }
                else
                {
                    bPlayed = true; // 전략에 의해 차단됨 (조기 종료 플래그)
                }
            }
            break;
        }
    }
    
    // 전략에 의해 명시적으로 차단되었다면 디코딩 연산 자체를 스킵
    if (bPlayed && !TargetReceiver) return;

    // 발화자를 월드에서 찾지 못했을 때의 Fallback (내 리시버에서 재생)
    if (!TargetReceiver)
    {
        TargetReceiver = AudioReceiver;
    }

    if (!TargetReceiver || !SpeakerCodec) return;

    // =========================================================================
    // 비동기 디코딩 (Async Pipeline)
    // - opus_decode는 UObject 미접근 → 백그라운드 스레드에서 실행 가능
    // - PlayVoice(QueueAudio)는 GameThread에서 실행 필수
    // =========================================================================
    TWeakObjectPtr<UVoiceCodec> WeakCodec = SpeakerCodec;
    TWeakObjectPtr<UVoiceAudioReceiver> WeakReceiver = TargetReceiver;
    TArray<uint8> VoiceCopy = VoiceData;

    AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask,
        [WeakCodec, WeakReceiver, VoiceCopy = MoveTemp(VoiceCopy)]()
    {
        if (!WeakCodec.IsValid()) return;

        TArray<uint8> PCMData;
        if (!WeakCodec->Decode(VoiceCopy, PCMData) || PCMData.Num() == 0)
        {
            return; // 디코딩 실패 → 깨진 패킷 드롭
        }

        // 디코딩된 PCM을 GameThread로 전달하여 재생
        AsyncTask(ENamedThreads::GameThread,
            [WeakReceiver, PCMData = MoveTemp(PCMData)]()
        {
            if (WeakReceiver.IsValid())
            {
                WeakReceiver->PlayVoice(PCMData);
            }
        });
    });
}

