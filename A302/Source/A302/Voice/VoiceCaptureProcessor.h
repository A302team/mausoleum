#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Interfaces/VoiceCapture.h"
#include "AudioCaptureBlueprintLibrary.h"
#include "AudioCapture.h"
#include "AudioCaptureComponent.h"
#include "VoiceCaptureProcessor.generated.h"

DECLARE_DELEGATE_OneParam(FOnVoiceDataCaptured, const FString& /*EncodedPayload*/);

UCLASS()
class A302_API UVoiceCaptureProcessor : public UObject
{
    GENERATED_BODY()

public:
    void Initialize();
    void Start(UWorld* World);
    void Stop(UWorld* World);
    void ToggleMicrophone(UWorld* World);
    
    bool ChangeMicrophone(const FString& DeviceName);
    bool IsMicrophoneActive() const { return bIsMicActive; }

    FOnVoiceDataCaptured OnVoiceDataCaptured;

private:
    UFUNCTION()
    void OnAudioDevicesObtained(const TArray<FAudioInputDeviceInfo>& AvailableDevices);

    void ProcessCapture();

    TSharedPtr<IVoiceCapture> VoiceCapture;
    FTimerHandle CaptureTimer;
    bool bIsMicActive = false;
};
