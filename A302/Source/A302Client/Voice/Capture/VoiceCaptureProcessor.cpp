#include "Voice/Capture/VoiceCaptureProcessor.h"
#include "Voice/Profiling/VoiceProfiler.h"
#include "Voice/Codec/VoiceCodec.h"
#include "VoiceModule.h"
#include "Async/Async.h"
#include "Misc/Base64.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace
{
    // 알탭/프레임 정체 후 누적 캡처 데이터를 한 번에 밀어 보내지 않도록 상한을 둡니다.
    constexpr uint32 MaxCaptureReadBytesPerTick = 4096;
    constexpr uint32 CaptureDrainChunkBytes = 1024;
}

void UVoiceCaptureProcessor::Initialize()
{
    bIsMicActive = false;

    // Opus 코덱 초기화
    Codec = NewObject<UVoiceCodec>(this);
    if (Codec)
    {
        Codec->Init();
    }
    
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

    World->GetTimerManager().SetTimer(CaptureTimer, this, &UVoiceCaptureProcessor::ProcessCapture, 0.02f, true);
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
        UE_LOG(LogVoiceChat, Log, TEXT("마이크 OFF"));
        Stop(World);
    }
    else
    {
        Start(World);
        if (bIsMicActive)
        {
            UE_LOG(LogVoiceChat, Log, TEXT("마이크 ON"));
        }
        else
        {
            UE_LOG(LogVoiceChat, Error, TEXT("[VoiceCaptureProcessor] 마이크 켜기 실패! VoiceCapture가 유효하지 않습니다. 마이크 장치를 확인하세요."));
        }
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
        UE_LOG(LogVoiceChat, Log, TEXT("마이크 장치 변경: %s"), *DeviceName);
        // 이미 켜져 있던 상태면 다시 시작 (단, 이 시점엔 World Timer 갱신은 상위에서 챙겨야 하거나 Capture 시도함. 일단 Start만 호출)
        if (bIsMicActive)
        {
            VoiceCapture->Start();
        }
        return true;
    }
    else
    {
        UE_LOG(LogVoiceChat, Error, TEXT("[VoiceCaptureProcessor] 마이크 장치 변경 실패: %s"), *DeviceName);
        return false;
    }
}

void UVoiceCaptureProcessor::OnAudioDevicesObtained(const TArray<FAudioInputDeviceInfo>& AvailableDevices)
{
    if (!IsInGameThread())
    {
        TWeakObjectPtr<UVoiceCaptureProcessor> WeakThis(this);
        TArray<FAudioInputDeviceInfo> DevicesCopy = AvailableDevices;
        AsyncTask(ENamedThreads::GameThread, [WeakThis, DevicesCopy = MoveTemp(DevicesCopy)]() mutable
        {
            if (WeakThis.IsValid())
            {
                WeakThis->OnAudioDevicesObtained(DevicesCopy);
            }
        });
        return;
    }

    if (AvailableDevices.Num() > 0)
    {
        UE_LOG(LogVoiceChat, Log, TEXT("[VoiceCaptureProcessor] 감지된 마이크: '%s'"), *AvailableDevices[0].DeviceName);
    }
    else
    {
        UE_LOG(LogVoiceChat, Warning, TEXT("감지된 마이크 없음"));
    }

    // ※ 중요: CreateVoiceCapture는 DirectSound 디바이스 ID를 기대합니다.
    // GetAvailableAudioInputDevices가 반환하는 Friendly Name("마이크(Realtek(R) Audio)")은
    // DirectSound 목록에서 매칭되지 않으므로 반드시 빈 문자열(=OS 기본 장치)을 사용해야 합니다.
    VoiceCapture = FVoiceModule::Get().CreateVoiceCapture(TEXT(""));

    if (VoiceCapture.IsValid())
    {
        UE_LOG(LogVoiceChat, Log, TEXT("[VoiceCaptureProcessor] 마이크 캡처 객체 생성 완료!"));
    }
    else
    {
        UE_LOG(LogVoiceChat, Error, TEXT("[VoiceCaptureProcessor] 마이크 캡처 실패! Windows 설정 > 마이크 액세스 권한을 확인하세요."));
    }
}

void UVoiceCaptureProcessor::ProcessCapture()
{
    VOICE_PROFILE_CAPTURE();
    if (!bIsMicActive || !VoiceCapture.IsValid()) return;
    
    uint32 AvailableData = 0;
    EVoiceCaptureState::Type CaptureState = VoiceCapture->GetCaptureState(AvailableData);

    if (CaptureState == EVoiceCaptureState::Ok && AvailableData > 0)
    {
        if (AvailableData > MaxCaptureReadBytesPerTick)
        {
            uint32 BytesToDiscard = AvailableData - MaxCaptureReadBytesPerTick;
            TArray<uint8> DrainBuffer;
            DrainBuffer.SetNumUninitialized(CaptureDrainChunkBytes);

            while (BytesToDiscard > 0)
            {
                const uint32 RequestBytes = FMath::Min(BytesToDiscard, CaptureDrainChunkBytes);
                uint32 DiscardedBytes = 0;
                VoiceCapture->GetVoiceData(DrainBuffer.GetData(), RequestBytes, DiscardedBytes);
                if (DiscardedBytes == 0)
                {
                    break;
                }
                BytesToDiscard = (DiscardedBytes >= BytesToDiscard) ? 0 : (BytesToDiscard - DiscardedBytes);
            }

            uint32 RefreshedAvailableData = 0;
            if (VoiceCapture->GetCaptureState(RefreshedAvailableData) != EVoiceCaptureState::Ok || RefreshedAvailableData == 0)
            {
                return;
            }
            AvailableData = RefreshedAvailableData;
        }

        const uint32 RequestedReadBytes = FMath::Min(AvailableData, MaxCaptureReadBytesPerTick);
        TArray<uint8> VoiceBuffer;
        VoiceBuffer.SetNumUninitialized(RequestedReadBytes);
        
        uint32 ReadData = 0;
        
        // GetVoiceData는 RAW PCM을 반환합니다.
        VoiceCapture->GetVoiceData(VoiceBuffer.GetData(), RequestedReadBytes, ReadData);

        if (ReadData > 0)
        {
            VoiceBuffer.SetNum(ReadData);
            
            // PCM → Opus 압축
            if (Codec && Codec->IsInitialized())
            {
                TArray<uint8> OpusData;
                if (Codec->Encode(VoiceBuffer, OpusData) && OpusData.Num() > 0)
                {
                    OnVoiceDataCaptured.ExecuteIfBound(OpusData);
                }
            }
            else
            {
                // 코덱 미초기화 시 RAW PCM 그대로 전송 (fallback)
                OnVoiceDataCaptured.ExecuteIfBound(VoiceBuffer);
            }
        }
    }

}
