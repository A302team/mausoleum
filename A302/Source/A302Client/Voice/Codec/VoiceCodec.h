#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Containers/CircularQueue.h"
#include "VoiceCodec.generated.h"

struct OpusEncoder;
struct OpusDecoder;

/**
 * Raw libopus를 직접 사용하는 Stateless Opus 코덱
 * - 각 Encode/Decode 호출이 독립적인 프레임을 생성 → UDP에 적합
 * - UE5의 IVoiceEncoder/IVoiceDecoder는 Stream 기반이라 UDP에서 상태가 깨짐
 * - Lock-free TCircularQueue를 사용하여 O(1) 삽입/삭제 및 컨텍스트 스위칭 지연 최소화
 */
UCLASS()
class A302CLIENT_API UVoiceCodec : public UObject
{
    GENERATED_BODY()

public:
    void Init(int32 InSampleRate = 16000, int32 InChannels = 1);
    void Shutdown();

    bool Encode(const TArray<uint8>& InPCM, TArray<uint8>& OutOpus);
    bool Decode(const TArray<uint8>& InOpus, TArray<uint8>& OutPCM);

    bool IsInitialized() const { return bInitialized; }
    int32 GetSampleRate() const { return SampleRate; }
    int32 GetChannels() const { return Channels; }

    virtual void BeginDestroy() override;

private:
    OpusEncoder* Encoder = nullptr;
    OpusDecoder* Decoder = nullptr;

    int32 SampleRate = 16000;
    int32 Channels = 1;
    int32 FrameSize = 320; // 20ms @ 16kHz
    int32 MaxBufferFrames = 3; // 최대 3프레임(60ms) 분량만 유지
    bool bInitialized = false;

    // Lock-free 순환 큐 (TCircularQueue는 기본 생성자가 없으므로 TUniquePtr로 래핑)
    TUniquePtr<TCircularQueue<uint8>> PCMRingBuffer;
};
