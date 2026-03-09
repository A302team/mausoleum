#include "Voice/Codec/VoiceCodec.h"
#include "Voice/Profiling/VoiceProfiler.h"

THIRD_PARTY_INCLUDES_START
#include "opus.h"
THIRD_PARTY_INCLUDES_END

void UVoiceCodec::Init(int32 InSampleRate, int32 InChannels)
{
    Shutdown(); // 이전 인스턴스 정리

    SampleRate = InSampleRate;
    Channels = InChannels;
    FrameSize = SampleRate / 50; // 20ms 프레임 (16000/50 = 320)

    // Ring Buffer 생성: 최대 MaxBufferFrames 프레임 분량 + 여유분
    const int32 BytesPerFrame = FrameSize * Channels * sizeof(opus_int16);
    const int32 RingCapacity = BytesPerFrame * (MaxBufferFrames + 1); // +1 프레임 여유
    PCMRingBuffer = MakeUnique<TCircularQueue<uint8>>(RingCapacity);

    int32 Err = 0;

    Encoder = opus_encoder_create(SampleRate, Channels, OPUS_APPLICATION_VOIP, &Err);
    if (Err != OPUS_OK || !Encoder)
    {
        UE_LOG(LogVoiceChat, Error, TEXT("[VoiceCodec] Opus 인코더 생성 실패: %d"), Err);
        return;
    }

    // VoIP 최적화 설정
    opus_encoder_ctl(Encoder, OPUS_SET_BITRATE(24000));          // 24kbps (16kHz 음성에 충분)
    opus_encoder_ctl(Encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(Encoder, OPUS_SET_INBAND_FEC(1));           // 패킷 로스 복원
    opus_encoder_ctl(Encoder, OPUS_SET_PACKET_LOSS_PERC(10));    // 10% 패킷 로스 예상
    opus_encoder_ctl(Encoder, OPUS_SET_DTX(1));                  // 무음 시 패킷 전송 중단 (대역폭 절약)
    opus_encoder_ctl(Encoder, OPUS_SET_COMPLEXITY(5));            // 인코딩 복잡도 낮춰 CPU 절약

    Decoder = opus_decoder_create(SampleRate, Channels, &Err);
    if (Err != OPUS_OK || !Decoder)
    {
        UE_LOG(LogVoiceChat, Error, TEXT("[VoiceCodec] Opus 디코더 생성 실패: %d"), Err);
        opus_encoder_destroy(Encoder);
        Encoder = nullptr;
        return;
    }

    bInitialized = true;
    UE_LOG(LogVoiceChat, Log, TEXT("[VoiceCodec] Opus 초기화 완료 (SampleRate=%d, Channels=%d, FrameSize=%d, RingCapacity=%d)"), SampleRate, Channels, FrameSize, RingCapacity);
}

void UVoiceCodec::Shutdown()
{
    if (Encoder)
    {
        opus_encoder_destroy(Encoder);
        Encoder = nullptr;
    }
    if (Decoder)
    {
        opus_decoder_destroy(Decoder);
        Decoder = nullptr;
    }
    PCMRingBuffer.Reset();
    bInitialized = false;
}

void UVoiceCodec::BeginDestroy()
{
    Shutdown();
    Super::BeginDestroy();
}

bool UVoiceCodec::Encode(const TArray<uint8>& InPCM, TArray<uint8>& OutOpus)
{
    VOICE_PROFILE_ENCODE();
    if (!bInitialized || !Encoder || InPCM.Num() == 0 || !PCMRingBuffer) return false;

    const int32 BytesPerFrame = FrameSize * Channels * sizeof(opus_int16);

    // ===== 1. 새 PCM 데이터를 Ring Buffer에 Enqueue =====
    for (int32 i = 0; i < InPCM.Num(); i++)
    {
        if (!PCMRingBuffer->Enqueue(InPCM[i]))
        {
            // 큐가 가득 찬 경우 → 가장 오래된 데이터를 폐기하여 공간 확보
            uint8 Dummy;
            PCMRingBuffer->Dequeue(Dummy);
            PCMRingBuffer->Enqueue(InPCM[i]);
        }
    }

    // ===== 2. 프레임 단위로 충분한 데이터가 쌓였는지 확인 =====
    if (static_cast<int32>(PCMRingBuffer->Count()) < BytesPerFrame)
    {
        return false; // 아직 1프레임(640B) 미만 → 인코딩 보류
    }

    UE_LOG(LogVoiceChat, Verbose, TEXT("[VoiceCodec] Encode 시작: RingBuffer에 %d 바이트 (필요 %d 바이트)"),
        PCMRingBuffer->Count(), BytesPerFrame);

    // ===== 3. Ring Buffer에서 1프레임씩 Dequeue → Opus 인코딩 =====
    OutOpus.Reset();
    TArray<uint8> TempBuffer;
    TempBuffer.SetNumUninitialized(4000); // 단일 프레임 최대 크기
    TArray<uint8> FrameBuffer;
    FrameBuffer.SetNumUninitialized(BytesPerFrame);

    while (static_cast<int32>(PCMRingBuffer->Count()) >= BytesPerFrame)
    {
        // 1프레임 분량을 연속 메모리로 Dequeue (opus_encode에 연속 포인터 필요)
        for (int32 i = 0; i < BytesPerFrame; i++)
        {
            PCMRingBuffer->Dequeue(FrameBuffer[i]);
        }

        const opus_int16* FramePtr = reinterpret_cast<const opus_int16*>(FrameBuffer.GetData());
        int32 EncodedBytes = opus_encode(Encoder, FramePtr, FrameSize, TempBuffer.GetData(), TempBuffer.Num());

        if (EncodedBytes < 0)
        {
            UE_LOG(LogVoiceChat, Error, TEXT("[VoiceCodec] Encode 오류 발생! 에러코드: %d"), EncodedBytes);
        }
        else if (EncodedBytes > 0)
        {
            // [2바이트: 프레임 길이][N바이트: Opus 데이터] 형식으로 패킹
            uint16 FrameLen = static_cast<uint16>(EncodedBytes);
            OutOpus.Append(reinterpret_cast<uint8*>(&FrameLen), sizeof(uint16));
            OutOpus.Append(TempBuffer.GetData(), EncodedBytes);

            UE_LOG(LogVoiceChat, Verbose, TEXT("Encode: PCM %d -> Opus %dB"), BytesPerFrame, EncodedBytes);
        }
    }

    return OutOpus.Num() > 0;
}

bool UVoiceCodec::Decode(const TArray<uint8>& InOpus, TArray<uint8>& OutPCM)
{
    VOICE_PROFILE_DECODE();
    if (!bInitialized || !Decoder || InOpus.Num() == 0) return false;

    OutPCM.Reset();
    TArray<opus_int16> DecodedSamples;
    DecodedSamples.SetNumUninitialized(FrameSize * Channels);

    int32 Offset = 0;
    while (Offset + sizeof(uint16) <= InOpus.Num())
    {
        // [2바이트: 프레임 길이] 읽기
        uint16 FrameLen = 0;
        FMemory::Memcpy(&FrameLen, InOpus.GetData() + Offset, sizeof(uint16));
        Offset += sizeof(uint16);

        if (FrameLen == 0 || Offset + FrameLen > InOpus.Num())
        {
            UE_LOG(LogVoiceChat, Error, TEXT("[VoiceCodec] Decode 프레임 길이 오류! 읽은 길이: %d, 남은 버퍼: %d. 패킷 손상이 의심됩니다."), FrameLen, InOpus.Num() - Offset);
            break; // 잘못된 프레임 → 중단
        }

        int32 DecodedCount = opus_decode(Decoder, InOpus.GetData() + Offset, FrameLen, DecodedSamples.GetData(), FrameSize, 0);

        if (DecodedCount < 0)
        {
            UE_LOG(LogVoiceChat, Error, TEXT("[VoiceCodec] Decode 실패! 원인코드: %d, 처리한 FrameLen: %d"), DecodedCount, FrameLen);
        }

        Offset += FrameLen;

        if (DecodedCount > 0)
        {
            int32 PCMBytes = DecodedCount * Channels * sizeof(opus_int16);
            OutPCM.Append(reinterpret_cast<uint8*>(DecodedSamples.GetData()), PCMBytes);
            UE_LOG(LogVoiceChat, Verbose, TEXT("Decode: Opus %d -> PCM %dB"), FrameLen, PCMBytes);
        }
    }

    if (OutPCM.Num() > 0)
    {
        UE_LOG(LogVoiceChat, Verbose, TEXT("Decode 완료: PCM %dB"), OutPCM.Num());
    }
    else
    {
        UE_LOG(LogVoiceChat, Verbose, TEXT("Decode: 반환 PCM 0B (무음 또는 프레임 부족)"));
    }

    return OutPCM.Num() > 0;
}
