#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "VoiceAudioReceiver.generated.h"

UCLASS()
class A302CLIENT_API UVoiceAudioReceiver : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UActorComponent* OuterComp);
    void PlayVoice(const TArray<uint8>& VoiceData);

private:
    UPROPERTY(VisibleAnywhere, Category = "Voice|Audio")
    TObjectPtr<UAudioComponent> AudioComponent = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<USoundWaveProcedural> SoundWave = nullptr;
};
