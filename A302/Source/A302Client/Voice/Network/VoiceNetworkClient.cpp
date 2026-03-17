#include "Voice/Network/VoiceNetworkClient.h"
#include "Network/GameNetworkSubsystem.h"
#include "Voice/Profiling/VoiceProfiler.h"

#pragma pack(push, 1)
struct FVoicePacketHeader {
    uint8_t packetType;       // 0: Join, 1: Voice Data, 2: Leave
    char roomCode[16];        // 룸 코드
    char speakerName[32];     // 발화자 이름
    uint32_t payloadSize;     // 뒤에 따라오는 음성 데이터 
};
#pragma pack(pop)

void UVoiceNetworkClient::Initialize(UObject* OuterObj)
{
    if (!OuterObj) return;

    UGameInstance* GameInstance = OuterObj->GetWorld() ? OuterObj->GetWorld()->GetGameInstance() : nullptr;
    if (GameInstance)
    {
        GameNetworkSubsystem = Cast<UGameNetworkSubsystem>(GameInstance->GetSubsystemBase(UGameNetworkSubsystem::StaticClass()));
        if (GameNetworkSubsystem)
        {
            // UDP 바이너리 수신 리스너
            GameNetworkSubsystem->OnBinaryPacketReceived.AddUObject(this, &UVoiceNetworkClient::HandleBinaryMessage);
        }
    }
}

void UVoiceNetworkClient::Connect(const FString& URL)
{
    if (GameNetworkSubsystem && !URL.IsEmpty())
    {
        if (!GameNetworkSubsystem->IsConnected(EProtocolType::UDP))
        {
            GameNetworkSubsystem->Connect(EProtocolType::UDP, URL);
        }
    }
}

void UVoiceNetworkClient::Disconnect(const FString& RoomCode, const FString& SpeakerName)
{
    SendLeavePacket(RoomCode, SpeakerName);
}

bool UVoiceNetworkClient::IsConnected() const
{
    return GameNetworkSubsystem && GameNetworkSubsystem->IsConnected(EProtocolType::UDP);
}

void UVoiceNetworkClient::SendVoiceData(const TArray<uint8>& Payload, const FString& RoomCode, const FString& Mode, const FString& SpeakerName)
{
    VOICE_PROFILE_NET_SEND();
    if (!GameNetworkSubsystem || !IsConnected()) return;

    // 헤더 값 설정
    FVoicePacketHeader Header;
    FMemory::Memzero(&Header, sizeof(FVoicePacketHeader));

    Header.packetType = (Mode == TEXT("join_voice")) ? 0 : 1;
    
    FTCHARToUTF8 Utf8RoomCode(*RoomCode);
    FMemory::Memcpy(Header.roomCode, Utf8RoomCode.Get(), FMath::Min(Utf8RoomCode.Length(), 15));
    
    FTCHARToUTF8 Utf8SpeakerName(*SpeakerName);
    FMemory::Memcpy(Header.speakerName, Utf8SpeakerName.Get(), FMath::Min(Utf8SpeakerName.Length(), 31));

    Header.payloadSize = Payload.Num();

    // 헤더 + 페이로드를 하나의 연속된 바이트 배열로 합치기
    TArray<uint8> BinaryData;
    BinaryData.SetNumUninitialized(sizeof(FVoicePacketHeader) + Header.payloadSize);

    FMemory::Memcpy(BinaryData.GetData(), &Header, sizeof(FVoicePacketHeader));

    if (Header.payloadSize > 0)
    {
        FMemory::Memcpy(BinaryData.GetData() + sizeof(FVoicePacketHeader), Payload.GetData(), Header.payloadSize);
    }

    if (Header.payloadSize > 2 || Header.packetType == 1)
    {
        UE_LOG(LogVoiceChat, Verbose, TEXT("Send: room=%s speaker=%s total=%dB payload=%dB"), *RoomCode, *SpeakerName, BinaryData.Num(), Header.payloadSize);
    }
    GameNetworkSubsystem->SendBinaryPacket(EProtocolType::UDP, BinaryData);
}

void UVoiceNetworkClient::SendLeavePacket(const FString& RoomCode, const FString& SpeakerName)
{
    if (!GameNetworkSubsystem || !IsConnected()) return;

    FVoicePacketHeader Header;
    FMemory::Memzero(&Header, sizeof(FVoicePacketHeader));
    Header.packetType = 2; // Leave
    Header.payloadSize = 0;

    FTCHARToUTF8 Utf8RoomCode(*RoomCode);
    FMemory::Memcpy(Header.roomCode, Utf8RoomCode.Get(), FMath::Min(Utf8RoomCode.Length(), 15));

    FTCHARToUTF8 Utf8SpeakerName(*SpeakerName);
    FMemory::Memcpy(Header.speakerName, Utf8SpeakerName.Get(), FMath::Min(Utf8SpeakerName.Length(), 31));

    TArray<uint8> BinaryData;
    BinaryData.SetNumUninitialized(sizeof(FVoicePacketHeader));
    FMemory::Memcpy(BinaryData.GetData(), &Header, sizeof(FVoicePacketHeader));

    UE_LOG(LogVoiceChat, Log, TEXT("[Voice] Leave 패킷 전송 (Room: %s, User: %s)"), *RoomCode, *SpeakerName);
    GameNetworkSubsystem->SendBinaryPacket(EProtocolType::UDP, BinaryData);
}

void UVoiceNetworkClient::HandleBinaryMessage(const TArray<uint8>& BinaryData)
{
    VOICE_PROFILE_NET_RECV();
    if (BinaryData.Num() < sizeof(FVoicePacketHeader)) return;

    const FVoicePacketHeader* Header = reinterpret_cast<const FVoicePacketHeader*>(BinaryData.GetData());
    
    // Voice Data 패킷(1)만 처리
    if (Header->packetType != 1) return;

    FString RoomCode = UTF8_TO_TCHAR(Header->roomCode);
    FString Speaker = UTF8_TO_TCHAR(Header->speakerName);
    
    if (Header->payloadSize > 0 && BinaryData.Num() >= (int32)(sizeof(FVoicePacketHeader) + Header->payloadSize))
    {
        const uint8_t* PayloadStart = BinaryData.GetData() + sizeof(FVoicePacketHeader);
        
        TArray<uint8> PayloadArray;
        PayloadArray.Append(PayloadStart, Header->payloadSize);
        
        OnBinaryPacketReceived.ExecuteIfBound(RoomCode, Speaker, PayloadArray);
    }
}
