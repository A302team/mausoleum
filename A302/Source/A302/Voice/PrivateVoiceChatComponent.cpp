#include "Voice/PrivateVoiceChatComponent.h"
#include "Voice/Capture/VoiceCaptureProcessor.h"
#include "Voice/Playback/VoiceAudioReceiver.h"
#include "Voice/Network/VoiceNetworkClient.h"
#include "Voice/Codec/VoiceCodec.h"
#include "Voice/Strategy/DistanceVoiceChatStrategy.h"
#include "Voice/Strategy/LobbyVoiceChatStrategy.h"
#include "Voice/Strategy/VoiceChatStrategyBase.h"
#include "GameMode/A302GameInstance.h"
#include "GameMode/LobbyGameMode.h"
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
    UE_LOG(LogTemp, Log, TEXT("[Voice] BeginPlay - Facade Initialize"));

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

    // Opus 디코더 초기화 (수신 시 Opus→PCM 변환용)
    ReceiveCodec = NewObject<UVoiceCodec>(this);
    if (ReceiveCodec)
    {
        ReceiveCodec->Init();
    }

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
            UE_LOG(LogTemp, Log, TEXT("[Voice] GameMode=Lobby → 로비 보이스 모드 적용"));
        }
        else
        {
            SetDistanceMode(DefaultInGameHearingDistance);
            UE_LOG(LogTemp, Log, TEXT("[Voice] GameMode=InGame → 거리 보이스 모드 적용 (%.0f)"), DefaultInGameHearingDistance);
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
        UE_LOG(LogTemp, Log, TEXT("[Voice] ConnectToVoiceServer: join_voice 초기 가입 패킷 전송 완료."));
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
    UE_LOG(LogTemp, Warning, TEXT("[Voice] SetRoomCode Called: Old='%s' New='%s'"), *roomCode, *InRoomCode);
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

    // Opus → PCM 디코딩
    TArray<uint8> PCMData;
    if (ReceiveCodec && ReceiveCodec->IsInitialized())
    {
        if (!ReceiveCodec->Decode(VoiceData, PCMData))
        {
            return; // 디코딩 실패 → 깨진 패킷 드롭 (Opus 데이터를 PCM으로 재생하면 노이즈 발생)
        }
    }
    else
    {
        // 코덱 미초기화 → 원본 데이터가 이미 PCM이라 가정 (fallback)
        PCMData = VoiceData;
    }

    // 상대방 위치 찾아서 재생 라우팅
    bool bPlayed = false;
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
                    if (IsValid(OtherComp) && OtherComp->AudioReceiver)
                    {
                        OtherComp->AudioReceiver->PlayVoice(PCMData);
                        bPlayed = true;
                    }
                }
                else
                {
                    bPlayed = true; // 전략에 의해 차단 → Fallback 방지
                }
            }
            break;
        }
    }
    
    // 발화자 미발견 시 Fallback
    if (!bPlayed && AudioReceiver)
    {
        AudioReceiver->PlayVoice(PCMData);
    }
}

