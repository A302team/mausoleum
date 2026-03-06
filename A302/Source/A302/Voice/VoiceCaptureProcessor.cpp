#include "Voice/VoiceCaptureProcessor.h"
#include "VoiceModule.h"
#include "Misc/Base64.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UVoiceCaptureProcessor::Initialize()
{
    bIsMicActive = false;
    
    FOnAudioInputDevicesObtained OnDevicesObtained;
    OnDevicesObtained.BindDynamic(this, &UVoiceCaptureProcessor::OnAudioDevicesObtained);
    
    // 수정: Library 클래스 이름 변경
    UAudioCaptureBlueprintLibrary::GetAvailableAudioInputDevices(this, OnDevicesObtained);
}
void UVoiceCaptureProcessor::Start(UWorld* World)
{
    if (!VoiceCapture.IsValid() || !World) return;

    VoiceCapture->Start();
    bIsMicActive = true;

    World->GetTimerManager().SetTimer(CaptureTimer, this, &UVoiceCaptureProcessor::ProcessCapture, 0.05f, false);
}

void UVoiceCaptureProcessor::Stop(UWorld* World)
{
    if (!VoiceCapture.IsValid() || !World) return;

    VoiceCapture->Stop();
    bIsMicActive = false;

    World->GetTimerManager().ClearTimer(CaptureTimer);
}

void UVoiceCaptureProcessor::ToggleMicrophone(UWorld* World)
{
    static double LastToggleTime = 0.0;
    double CurrentTime = FPlatformTime::Seconds();
    if (CurrentTime - LastToggleTime < 0.2) return; // 200ms 디바운스 처리 (더블클릭 방지)
    LastToggleTime = CurrentTime;

    if (bIsMicActive)
    {
        UE_LOG(LogTemp, Warning, TEXT("[VoiceCaptureProcessor] 마이크 끄기 (OFF)"));
        Stop(World);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[VoiceCaptureProcessor] 마이크 켜기 (ON)"));
        Start(World);
    }
}

bool UVoiceCaptureProcessor::ChangeMicrophone(const FString& DeviceName)
{
    if (VoiceCapture.IsValid())
    {
        VoiceCapture->Stop();
        VoiceCapture.Reset();
    }

    VoiceCapture = FVoiceModule::Get().CreateVoiceCapture(DeviceName);

    if (VoiceCapture.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[VoiceCaptureProcessor] 마이크 장치 변경 성공: %s"), *DeviceName);
        // 이미 켜져 있던 상태면 다시 시작 (단, 이 시점엔 World Timer 갱신은 상위에서 챙겨야 하거나 Capture 시도함. 일단 Start만 호출)
        if (bIsMicActive)
        {
            VoiceCapture->Start();
        }
        return true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceCaptureProcessor] 마이크 장치 변경 실패: %s"), *DeviceName);
        return false;
    }
}

void UVoiceCaptureProcessor::OnAudioDevicesObtained(const TArray<FAudioInputDeviceInfo>& AvailableDevices)
{
    FString TargetDeviceName = TEXT("");

   if (AvailableDevices.Num() > 0)
    {
        // 수정: DeviceName은 이미 FString이므로 .ToString()이 필요 없습니다.
        TargetDeviceName = AvailableDevices[0].DeviceName; 
        UE_LOG(LogTemp, Warning, TEXT("[VoiceCaptureProcessor] 자동 감지 시스템: '%s' 마이크를 발견하여 연결을 시도합니다."), *TargetDeviceName);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[VoiceCaptureProcessor] 감지된 마이크가 없어 기본 통신 장치로 시도합니다."));
    }

    // 16000Hz, 1채널(모노)로 명시적 캡처 요청. (USoundWaveProcedural 설정과 일치시키기 위해)
    VoiceCapture = FVoiceModule::Get().CreateVoiceCapture(TargetDeviceName, 16000, 1);

    if (!VoiceCapture.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceCaptureProcessor] 지정된 마이크 캡처 실패. 윈도우 기본 장치로 재시도합니다."));
        VoiceCapture = FVoiceModule::Get().CreateVoiceCapture("", 16000, 1);
    }

    if (VoiceCapture.IsValid())
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceCaptureProcessor] 마이크 캡처 객체 세팅 완료!"));
    }
}

void UVoiceCaptureProcessor::ProcessCapture()
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
            OnVoiceDataCaptured.ExecuteIfBound(EncodedString);
        }
    }

    // 다음 호출 등록
    if (bIsMicActive)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(CaptureTimer, this, &UVoiceCaptureProcessor::ProcessCapture, 0.01f, false);
        }
    }
}
