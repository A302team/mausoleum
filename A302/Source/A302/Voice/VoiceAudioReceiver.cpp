#include "Voice/VoiceAudioReceiver.h"
#include "Misc/Base64.h"

void UVoiceAudioReceiver::Initialize(UActorComponent* OuterComp)
{
    if (!OuterComp) return;

    SoundWave = NewObject<USoundWaveProcedural>(this);
    if (SoundWave)
    {
        SoundWave->SetSampleRate(16000); 
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
        
        // 거리 기반(Distance) 보이스를 위해 3D 공간화 지원 및 Owner 트랜스폼에 어태치
        if (AActor* OwnerActor = OuterComp->GetOwner())
        {
            if (USceneComponent* RootComp = OwnerActor->GetRootComponent())
            {
                AudioComponent->AttachToComponent(RootComp, FAttachmentTransformRules::KeepRelativeTransform);
            }
        }
        
        AudioComponent->SetSound(SoundWave);
        AudioComponent->bAllowSpatialization = true; // 3D 채팅을 위해 켬
        AudioComponent->Play();
    }
}

void UVoiceAudioReceiver::PlayVoice(const FString& EncodedPayload)
{
    TArray<uint8> DecodedVoiceData;
    if (FBase64::Decode(EncodedPayload, DecodedVoiceData))
    {
        if (SoundWave && DecodedVoiceData.Num() > 0)
        {
            SoundWave->QueueAudio(DecodedVoiceData.GetData(), DecodedVoiceData.Num());
            
            if (AudioComponent && !AudioComponent->IsPlaying())
            {
                UE_LOG(LogTemp, Warning, TEXT("[VoiceAudioReceiver] 오디오 컴포넌트가 멈춰있어서 다시 Play() 호출!"));
                AudioComponent->Play();
            }
        }
    }
}
