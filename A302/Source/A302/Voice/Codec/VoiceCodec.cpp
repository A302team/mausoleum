#include "Voice/Codec/VoiceCodec.h"

THIRD_PARTY_INCLUDES_START
#include "opus.h"
THIRD_PARTY_INCLUDES_END

void UVoiceCodec::Init(int32 InSampleRate, int32 InChannels)
{
    Shutdown(); // 이전 인스턴스 정리

    SampleRate = InSampleRate;
    Channels = InChannels;
    FrameSize = SampleRate / 50; // 20ms 프레임 (16000/50 = 320)

    int32 Err = 0;

    Encoder = opus_encoder_create(SampleRate, Channels, OPUS_APPLICATION_VOIP, &Err);
    if (Err != OPUS_OK || !Encoder)
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceCodec] Opus 인코더 생성 실패: %d"), Err);
        return;
    }

    // VoIP 최적화 설정
    opus_encoder_ctl(Encoder, OPUS_SET_BITRATE(24000));          // 24kbps (16kHz 음성에 충분)
    opus_encoder_ctl(Encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(Encoder, OPUS_SET_INBAND_FEC(1));           // 패킷 로스 복원
    opus_encoder_ctl(Encoder, OPUS_SET_PACKET_LOSS_PERC(10));    // 10% 패킷 로스 예상

    Decoder = opus_decoder_create(SampleRate, Channels, &Err);
    if (Err != OPUS_OK || !Decoder)
    {
        UE_LOG(LogTemp, Error, TEXT("[VoiceCodec] Opus 디코더 생성 실패: %d"), Err);
        opus_encoder_destroy(Encoder);
        Encoder = nullptr;
        return;
    }

    bInitialized = true;
    UE_LOG(LogTemp, Log, TEXT("[VoiceCodec] Opus 초기화 완료 (SampleRate=%d, Channels=%d, FrameSize=%d)"), SampleRate, Channels, FrameSize);
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
    bInitialized = false;
}

void UVoiceCodec::BeginDestroy()
{
    Shutdown();
    Super::BeginDestroy();
}

bool UVoiceCodec::Encode(const TArray<uint8>& InPCM, TArray<uint8>& OutOpus)
{
    if (!bInitialized || !Encoder || InPCM.Num() == 0) return false;

    // 새로 들어온 PCM 데이터를 내부에 누적
    PCMBuffer.Append(InPCM);

    const int32 BytesPerFrame = FrameSize * Channels * sizeof(opus_int16);
    
    // 아직 1프레임(320바이트) 용량이 안 모였으면 인코딩 보류
    if (PCMBuffer.Num() < BytesPerFrame)
    {
        // 모자라면 스킵
        return false;
    }

    // 버퍼가 과도하게 쌓이는 것 방지 (최대 1초 분량 = 16000 * 1 * 2 = 32000바이트)
    // 랙이 걸려 버퍼가 무한정 쌓인다면 마이크 입력이 먹통되는 것을 방지하기 위해 오래된 데이터를 강제 삭제
    int32 MaxBufferSize = SampleRate * Channels * sizeof(opus_int16); // 1초 분량
    if (PCMBuffer.Num() > MaxBufferSize)
    {
        UE_LOG(LogTemp, Warning, TEXT("[VoiceCodec] 버퍼 초과 (%d bytes)! 오래된 오디오 폐기."), PCMBuffer.Num());
        int32 BytesToDrop = PCMBuffer.Num() - MaxBufferSize;
        PCMBuffer.RemoveAt(0, BytesToDrop, EAllowShrinking::No);
    }

    UE_LOG(LogTemp, Warning, TEXT("[VoiceCodec] Encode 시작: 버퍼에 %d 바이트 있음 (필요 %d 바이트)"), PCMBuffer.Num(), BytesPerFrame);

    OutOpus.Reset();
    TArray<uint8> TempBuffer;
    TempBuffer.SetNumUninitialized(4000); // 단일 프레임 최대 크기

    int32 Offset = 0;
    
    // 버퍼에 1920바이트 이상 있을 때마다 루프 돌면서 프레임 추출
    while (Offset + BytesPerFrame <= PCMBuffer.Num())
    {
        const opus_int16* FramePtr = reinterpret_cast<const opus_int16*>(PCMBuffer.GetData() + Offset);

        int32 EncodedBytes = opus_encode(Encoder, FramePtr, FrameSize, TempBuffer.GetData(), TempBuffer.Num());

        if (EncodedBytes < 0)
        {
            UE_LOG(LogTemp, Error, TEXT("[VoiceCodec] Encode 오류 발생! 에러코드: %d"), EncodedBytes);
        }
        else if (EncodedBytes > 0)
        {
            // [2바이트: 프레임 길이][N바이트: Opus 데이터] 형식으로 패킹
            uint16 FrameLen = static_cast<uint16>(EncodedBytes);
            OutOpus.Append(reinterpret_cast<uint8*>(&FrameLen), sizeof(uint16));
            OutOpus.Append(TempBuffer.GetData(), EncodedBytes);
            
            UE_LOG(LogTemp, Log, TEXT("[VoiceCodec] 프레임 인코딩 성공! PCM %d -> Opus %d 바이트"), BytesPerFrame, EncodedBytes);
        }

        Offset += BytesPerFrame;
    }

    // 처리 완료된 데이터는 버퍼에서 제거, 남은 쪼가리 데이터는 다음 틱으로 이월
    if (Offset > 0)
    {
        PCMBuffer.RemoveAt(0, Offset, EAllowShrinking::No);
    }

    return OutOpus.Num() > 0;
}

bool UVoiceCodec::Decode(const TArray<uint8>& InOpus, TArray<uint8>& OutPCM)
{
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
            UE_LOG(LogTemp, Error, TEXT("[VoiceCodec] Decode 프레임 길이 오류! 읽은 길이: %d, 남은 버퍼: %d. 패킷 손상이 의심됩니다."), FrameLen, InOpus.Num() - Offset);
            break; // 잘못된 프레임 → 중단
        }

        int32 DecodedCount = opus_decode(Decoder, InOpus.GetData() + Offset, FrameLen, DecodedSamples.GetData(), FrameSize, 0);

        if (DecodedCount < 0)
        {
            UE_LOG(LogTemp, Error, TEXT("[VoiceCodec] Decode 실패! 원인코드: %d, 처리한 FrameLen: %d"), DecodedCount, FrameLen);
        }

        Offset += FrameLen;

        if (DecodedCount > 0)
        {
            int32 PCMBytes = DecodedCount * Channels * sizeof(opus_int16);
            OutPCM.Append(reinterpret_cast<uint8*>(DecodedSamples.GetData()), PCMBytes);
            UE_LOG(LogTemp, Log, TEXT("[VoiceCodec] 프레임 디코딩 성공! Opus %d -> PCM %d 바이트"), FrameLen, PCMBytes);
        }
    }

    if (OutPCM.Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[VoiceCodec] Decode 완료. 최종 반환 PCM: %d 바이트"), OutPCM.Num());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[VoiceCodec] Decode 완료했으나 반환할 PCM이 0바이트입니다."));
    }

    return OutPCM.Num() > 0;
}
