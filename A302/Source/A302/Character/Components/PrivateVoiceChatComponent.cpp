#include "Character/Components/PrivateVoiceChatComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "Serialization/JsonReader.h"
#include "Dom/JsonObject.h"
#include "Network/WebSocketManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Voice/Strategy/DistanceVoiceChatStrategy.h"
#include "Voice/Strategy/LobbyVoiceChatStrategy.h"
#include "Voice/Strategy/VoiceChatStrategyBase.h"
#include "VoiceModule.h"
#include "Misc/Base64.h"
#include "TimerManager.h"

UPrivateVoiceChatComponent::UPrivateVoiceChatComponent()
{
    // 최적화를 위해 매 프레임 실행되는 Tick 기능 비활성화
    PrimaryComponentTick.bCanEverTick = false; 
    PrimaryComponentTick.bStartWithTickEnabled = false; 
}

void UPrivateVoiceChatComponent::StartMicrophone()
{
    if (!VoiceCapture.IsValid()) return;

    VoiceCapture->Start();
    bIsMicActive = true;

    // 반복(Loop)을 끄고(false), 0.05초 뒤에 "단 한 번만" 실행되도록 예약합니다. (함수 내부에서 꼬리물기)
    GetWorld()->GetTimerManager().SetTimer(VoiceCaptureTimer, this, &UPrivateVoiceChatComponent::ProcessVoiceCapture, 0.01f, false);
}

void UPrivateVoiceChatComponent::StopMicrophone()
{
    if (!VoiceCapture.IsValid()) return;

    VoiceCapture->Stop();
    bIsMicActive = false;

    // 마이크가 꺼지면 불필요한 타이머 중지
    GetWorld()->GetTimerManager().ClearTimer(VoiceCaptureTimer);
}

void UPrivateVoiceChatComponent::ToggleMicrophone()
{
    UE_LOG(LogTemp, Warning, TEXT("[Voice] ToggleMicrophone called. 현재 상태: %s"), bIsMicActive ? TEXT("ON") : TEXT("OFF"));

    if (bIsMicActive)
    {
        StopMicrophone();
    }
    else
    {
        StartMicrophone();
    }
}

// Tick 역할을 대신하는 최적화된 마이크 수집 함수
void UPrivateVoiceChatComponent::ProcessVoiceCapture()
{
    if (!bIsMicActive || !VoiceCapture.IsValid()) return;
    
    uint32 AvailableData = 0;
    EVoiceCaptureState::Type CaptureState = VoiceCapture->GetCaptureState(AvailableData);

    if (CaptureState == EVoiceCaptureState::Ok && AvailableData > 0)
    {
        TArray<uint8> VoiceBuffer;
        VoiceBuffer.SetNumUninitialized(AvailableData);
        
        uint32 ReadData = 0;
        
        VoiceCapture->GetVoiceData(VoiceBuffer.GetData(), AvailableData, ReadData);

        if (ReadData > 0)
        {
            FString EncodedString = FBase64::Encode(VoiceBuffer.GetData(), ReadData);
            SendVoicePayload(EncodedString);
        }
    }

    // [핵심] 처리가 끝난 후, 0.05초 뒤에 이 함수를 다시 실행하도록 재예약! (꼬리물기)
    if (bIsMicActive)
    {
        GetWorld()->GetTimerManager().SetTimer(VoiceCaptureTimer, this, &UPrivateVoiceChatComponent::ProcessVoiceCapture, 0.05f, false);
    }
}

void UPrivateVoiceChatComponent::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Log, TEXT("[Voice] BeginPlay"));

    if (!WebSocketManager)
    {
        WebSocketManager = NewObject<UWebSocketManager>(this);
        if (WebSocketManager)
        {
            WebSocketManager->OnMessageReceived.AddDynamic(this, &UPrivateVoiceChatComponent::HandleServerMessage);
        }
    }

    if (!LobbyStrategy)
    {
        LobbyStrategy = NewObject<ULobbyVoiceChatStrategy>(this);
    }

    if (!DistanceStrategy)
    {
        DistanceStrategy = NewObject<UDistanceVoiceChatStrategy>(this);
    }

    if (DistanceStrategy)
    {
        DistanceStrategy->SetHearingDistance(DefaultInGameHearingDistance);
    }

    if (InitialVoiceMode == EVoiceChatMode::Lobby)
    {
        SetLobbyMode();
    }
    else
    {
        SetDistanceMode(DefaultInGameHearingDistance);
    }

    if (bAutoConnectOnBeginPlay)
    {
        ConnectToVoiceServer();
    }

    VoiceCapture = FVoiceModule::Get().CreateVoiceCapture("");
    if(!VoiceCapture.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Voice] Failed to create VoiceCapture instance! Voice chat will not work."));
    }

    // 오디오 재생을 위한 셋업
    VoiceSoundWave = NewObject<USoundWaveProcedural>(this);
    if (VoiceSoundWave)
    {
        VoiceSoundWave->SetSampleRate(16000); 
        VoiceSoundWave->NumChannels = 1;      
        VoiceSoundWave->Duration = 10000.f;   
        VoiceSoundWave->SoundGroup = SOUNDGROUP_Voice;
        
        // 무조건 무한 반복(Loop)하도록 설정해야 중간에 안 꺼집니다.
        VoiceSoundWave->bLooping = true;
        VoiceSoundWave->bProcedural = true;
    }

    VoiceAudioComponent = NewObject<UAudioComponent>(this);
    if (VoiceAudioComponent)
    {
        VoiceAudioComponent->RegisterComponent();
        VoiceAudioComponent->SetSound(VoiceSoundWave);
        // 3D 공간음향 임시 비활성화 (순수 재생 테스트용)
        VoiceAudioComponent->bAllowSpatialization = false; 
        VoiceAudioComponent->Play();
    }
    
    bIsMicActive = false; 
}

void UPrivateVoiceChatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    DisconnectFromVoiceServer();
    Super::EndPlay(EndPlayReason);
    
    // 혹시라도 남아있을 수 있는 타이머 제거
    GetWorld()->GetTimerManager().ClearTimer(VoiceCaptureTimer);

    if (VoiceCapture.IsValid())
    {
        VoiceCapture->Stop();
        VoiceCapture.Reset(); 
        UE_LOG(LogTemp, Log, TEXT("[Voice] 마이크 리소스 안전하게 해제됨"));
    }
}

void UPrivateVoiceChatComponent::ConnectToVoiceServer()
{
    if (!WebSocketManager || VoiceServerUrl.IsEmpty())
    {
        return;
    }

    if (!WebSocketManager->IsConnected())
    {
        WebSocketManager->Connect(VoiceServerUrl);
    }
}

void UPrivateVoiceChatComponent::DisconnectFromVoiceServer()
{
    if (!WebSocketManager)
    {
        return;
    }

    WebSocketManager->Disconnect();
}

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
    if (!ActiveStrategy)
    {
        return EVoiceChatMode::Lobby;
    }

    return ActiveStrategy->GetMode();
}

bool UPrivateVoiceChatComponent::CanHearActor(const AActor* SpeakerActor) const
{
    if (!SpeakerActor || !ActiveStrategy)
    {
        return false;
    }

    const UPrivateVoiceChatComponent* SpeakerComp = SpeakerActor->FindComponentByClass<UPrivateVoiceChatComponent>();
    if (!SpeakerComp)
    {
        return false;
    }

    return ActiveStrategy->CanReceiveVoice(this, SpeakerComp);
}

void UPrivateVoiceChatComponent::SendVoicePayload(const FString& EncodedPayload)
{
    if (!WebSocketManager || !WebSocketManager->IsConnected() || !ActiveStrategy)
    {
        return;
    }

    const FString ModeString = (GetCurrentMode() == EVoiceChatMode::Lobby) ? TEXT("lobby") : TEXT("distance");

    TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    JsonObject->SetStringField(TEXT("type"), TEXT("voice_data"));
    JsonObject->SetStringField(TEXT("mode"), ModeString);
    JsonObject->SetStringField(TEXT("roomCode"), roomCode);
    JsonObject->SetStringField(TEXT("speaker"), GetNameSafe(GetOwner()));
    JsonObject->SetStringField(TEXT("payload"), EncodedPayload);

    FString OutMessage;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutMessage);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    WebSocketManager->SendMessage(OutMessage);
}

void UPrivateVoiceChatComponent::HandleServerMessage(const FString& Message)
{
    // 기존처럼 UI 등을 위해 이벤트는 계속 쏴줍니다.
    OnVoiceServerMessage.Broadcast(Message);

    // 1. JSON 문자열 파싱 준비
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);

    // 2. JSON 파싱 성공 시
    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        // 0. 서버 메시지에 roomCode가 포함되어 있다면 갱신
        FString ServerRoomCode;
        if (JsonObject->TryGetStringField(TEXT("roomCode"), ServerRoomCode) && !ServerRoomCode.IsEmpty())
        {
            if (roomCode != ServerRoomCode)
            {
                SetRoomCode(ServerRoomCode);
            }
        }

        FString MsgType;
        // 타입이 voice_data 인지 확인
        if (JsonObject->TryGetStringField(TEXT("type"), MsgType) && MsgType == TEXT("voice_data"))
        {
            // 3. 내가 한 말이 다시 내 스피커로 들리는 메아리(Echo) 현상 방지
            FString SpeakerName;
            if (JsonObject->TryGetStringField(TEXT("speaker"), SpeakerName))
            {
                // ==========================================
                // [테스트용] 현재 혼자 테스트 중이라면 내 목소리가 돌아오는 것을 들어야 하므로 주석 처리해두었습니다.
                // 친구와 멀티플레이를 시작하실 때는 반드시 이 주석을 해제(활성화)해 주세요!!
                // ==========================================
                // if (SpeakerName == GetNameSafe(GetOwner()))
                // {
                //     return; 
                // }
            }

            // 4. Base64로 인코딩된 진짜 음성 데이터(payload) 추출
            FString EncodedPayload;
            if (JsonObject->TryGetStringField(TEXT("payload"), EncodedPayload))
            {
                // 5. Base64 문자열을 원래의 바이트 배열(uint8)로 복원
                TArray<uint8> DecodedVoiceData;
                if (FBase64::Decode(EncodedPayload, DecodedVoiceData))
                {
                    // 6. 복원된 데이터를 절차적 사운드 웨이브에 밀어 넣어서 스피커로 즉시 재생!
                    if (VoiceSoundWave && DecodedVoiceData.Num() > 0)
                    {
                        VoiceSoundWave->QueueAudio(DecodedVoiceData.GetData(), DecodedVoiceData.Num());
                        
                        // 큐에 데이터가 들어갔는데 만약 컴포넌트가 자고 있다면 깨워줍니다.
                        if (VoiceAudioComponent && !VoiceAudioComponent->IsPlaying())
                        {
                            UE_LOG(LogTemp, Warning, TEXT("[Voice] 오디오 컴포넌트가 멈춰있어서 다시 Play() 호출!"));
                            VoiceAudioComponent->Play();
                        }
                    }
                }
            }
        }
    }
}

void UPrivateVoiceChatComponent::ApplyStrategy(UVoiceChatStrategyBase* NewStrategy)
{
    if (!NewStrategy)
    {
        return;
    }

    ActiveStrategy = NewStrategy;
    OnVoiceModeChanged.Broadcast(ActiveStrategy->GetMode());
}