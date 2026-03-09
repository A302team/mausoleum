#include "Voice/Playback/VoiceAudioReceiver.h"
#include "Voice/Profiling/VoiceProfiler.h"
#include "Sound/SoundAttenuation.h"
#include "Async/Async.h"

void UVoiceAudioReceiver::Initialize(UActorComponent* OuterComp)
{
    if (!OuterComp) return;

    SoundWave = NewObject<USoundWaveProcedural>(this);
    if (SoundWave)
    {
        SoundWave->SetSampleRate(16000);  // UE5 Voice 모듈 캡처 기본값 (16kHz)과 일치
        SoundWave->NumChannels = 1;
        SoundWave->Duration = 10000.f;
        SoundWave->SoundGroup = SOUNDGROUP_Voice;
        SoundWave->bLooping = true;
        SoundWave->bProcedural = true;
    }

    AudioComponent = NewObject<UAudioComponent>(OuterComp);
    if (AudioComponent)
    {
        AudioComponent->RegisterComponent();
        
        // Owner 트랜스폼에 어태치 (3D 위치 추적)
        if (AActor* OwnerActor = OuterComp->GetOwner())
        {
            if (USceneComponent* RootComp = OwnerActor->GetRootComponent())
            {
                AudioComponent->AttachToComponent(RootComp, FAttachmentTransformRules::KeepRelativeTransform);
            }
        }
        
        AudioComponent->SetSound(SoundWave);

        // ===== 3D 스패셜라이제이션 + 거리 감쇠 설정 =====
        AudioComponent->bAllowSpatialization = true;
        AudioComponent->bOverrideAttenuation = true;

        FSoundAttenuationSettings AttenuationSettings;
        AttenuationSettings.bAttenuate = true;
        AttenuationSettings.bSpatialize = true;

        // 선형 감쇠: InnerRadius ~ FalloffDistance 사이에서 볼륨 1.0 → 0.0
        AttenuationSettings.DistanceAlgorithm = EAttenuationDistanceModel::Linear;
        AttenuationSettings.AttenuationShapeExtents = FVector(200.f); // InnerRadius: 200 이내 풀볼륨
        AttenuationSettings.FalloffDistance = 1800.f;                  // 2000 거리에서 무음

        AudioComponent->AttenuationOverrides = AttenuationSettings;

        AudioComponent->Play();
    }
}

void UVoiceAudioReceiver::PlayVoice(const TArray<uint8>& VoiceData)
{
    VOICE_PROFILE_PLAYBACK();
    if (SoundWave && VoiceData.Num() > 0)
    {
        // 1. 오디오 큐잉 (QueueAudio는 내부적으로 Thread-Safe)
        SoundWave->QueueAudio(VoiceData.GetData(), VoiceData.Num());
        UE_LOG(LogVoiceChat, Verbose, TEXT("PlayVoice: QueueAudio %dB"), VoiceData.Num());
        
        // 2. AudioComponent 제어
        if (AudioComponent && !AudioComponent->IsPlaying())
        {
            UE_LOG(LogVoiceChat, Log, TEXT("AudioComponent::Play 시작"));
            AudioComponent->Play();
        }
    }
}
