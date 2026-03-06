#include "Voice/PrivateVoiceChatComponent.h"
#include "Voice/VoiceCaptureProcessor.h"
#include "Voice/VoiceAudioReceiver.h"
#include "Voice/VoiceNetworkClient.h"
#include "Voice/Strategy/DistanceVoiceChatStrategy.h"
#include "Voice/Strategy/LobbyVoiceChatStrategy.h"
#include "Voice/Strategy/VoiceChatStrategyBase.h"
#include "GameMode/A302GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
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
        NetworkClient->OnPacketReceived.BindUObject(this, &UPrivateVoiceChatComponent::OnNetworkMessageReceived);
    }

    if (!LobbyStrategy)
        LobbyStrategy = NewObject<ULobbyVoiceChatStrategy>(this);

    if (!DistanceStrategy)
        DistanceStrategy = NewObject<UDistanceVoiceChatStrategy>(this);

    if (DistanceStrategy)
        DistanceStrategy->SetHearingDistance(DefaultInGameHearingDistance);

    if (InitialVoiceMode == EVoiceChatMode::Lobby)
        SetLobbyMode();
    else
        SetDistanceMode(DefaultInGameHearingDistance);

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

void UPrivateVoiceChatComponent::ConnectToVoiceServer()
{
    if (NetworkClient && !VoiceServerUrl.IsEmpty())
    {
        NetworkClient->Connect(VoiceServerUrl);
    }
}

void UPrivateVoiceChatComponent::DisconnectFromVoiceServer()
{
    if (NetworkClient) NetworkClient->Disconnect();
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
    return ActiveStrategy ? ActiveStrategy->GetMode() : EVoiceChatMode::Lobby;
}

bool UPrivateVoiceChatComponent::CanHearActor(const AActor* SpeakerActor) const
{
    if (!SpeakerActor || !ActiveStrategy) return false;

    const UPrivateVoiceChatComponent* SpeakerComp = SpeakerActor->FindComponentByClass<UPrivateVoiceChatComponent>();
    if (!SpeakerComp) return false;

    return ActiveStrategy->CanReceiveVoice(this, SpeakerComp);
}

void UPrivateVoiceChatComponent::SendVoicePayload(const FString& EncodedPayload)
{
    if (!NetworkClient || !ActiveStrategy) return;

    const FString ModeString = (GetCurrentMode() == EVoiceChatMode::Lobby) ? TEXT("lobby") : TEXT("distance");

    FString MyName = GetNameSafe(GetOwner());
    if (UA302GameInstance* GI = Cast<UA302GameInstance>(UGameplayStatics::GetGameInstance(this)))
    {
        if (!GI->MyPlayerName.IsEmpty())
        {
            MyName = GI->MyPlayerName;
        }
    }

    NetworkClient->SendVoiceData(EncodedPayload, roomCode, ModeString, MyName);
}

void UPrivateVoiceChatComponent::ApplyStrategy(UVoiceChatStrategyBase* NewStrategy)
{
    if (!NewStrategy) return;
    ActiveStrategy = NewStrategy;
    OnVoiceModeChanged.Broadcast(ActiveStrategy->GetMode());
}

void UPrivateVoiceChatComponent::OnVoiceDataCaptured(const FString& EncodedPayload)
{
    SendVoicePayload(EncodedPayload);
}

void UPrivateVoiceChatComponent::OnNetworkMessageReceived(const FString& JsonMessage)
{
    OnVoiceServerMessage.Broadcast(JsonMessage);

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonMessage);

    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        FString ServerRoomCode;
        if (JsonObject->TryGetStringField(TEXT("roomCode"), ServerRoomCode) && !ServerRoomCode.IsEmpty())
        {
            if (roomCode != ServerRoomCode)
            {
                SetRoomCode(ServerRoomCode);
            }
        }

        FString MsgType;
        if (JsonObject->TryGetStringField(TEXT("type"), MsgType) && MsgType == TEXT("voice_data"))
        {
            // 자신이름도 PlayerState에서 가져와야 일관됨.
            FString MyName = TEXT("Unknown");
            if (APawn* MyPawn = Cast<APawn>(GetOwner()))
            {
                if (IsValid(MyPawn))
                {
                    if (APlayerState* PS = MyPawn->GetPlayerState())
                    {
                        if (IsValid(PS)) 
                        {
                            MyName = PS->GetPlayerName();
                        }
                    }
                }
            }

            FString SpeakerName;
            if (JsonObject->TryGetStringField(TEXT("speaker"), SpeakerName))
            {
                // 로컬의 메아리 방지
                if (SpeakerName == MyName) return; 
            }

            FString EncodedPayload;
            if (JsonObject->TryGetStringField(TEXT("payload"), EncodedPayload))
            {
                // 상대방 위치에서 소리를 재생하도록 발화자를 찾아 라우팅합니다.
                bool bPlayed = false;
                if (UWorld* World = GetWorld())
                {
                    for (TActorIterator<APawn> It(World); It; ++It)
                    {
                        APawn* CurrentPawn = *It;
                        if (IsValid(CurrentPawn))
                        {
                            if (APlayerState* PS = CurrentPawn->GetPlayerState())
                            {
                                if (IsValid(PS) && PS->GetPlayerName() == SpeakerName) // 맞는 발화자 발견!
                                {
                                    if (UPrivateVoiceChatComponent* OtherComp = CurrentPawn->FindComponentByClass<UPrivateVoiceChatComponent>())
                                    {
                                        if (IsValid(OtherComp) && OtherComp->AudioReceiver)
                                        {
                                            OtherComp->AudioReceiver->PlayVoice(EncodedPayload);
                                            bPlayed = true;
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
                
                // 만약 발화자를 못 찾았다면(아직 거리가 멀어 리플리케이트 안됐거나 등) fallback 재생
                if (!bPlayed && AudioReceiver)
                {
                    AudioReceiver->PlayVoice(EncodedPayload);
                }
            }
        }
    }
}
