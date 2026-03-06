#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VoiceNetworkClient.generated.h"

DECLARE_DELEGATE_ThreeParams(FOnVoiceBinaryPacketReceived, const FString& /*RoomCode*/, const FString& /*Speaker*/, const TArray<uint8>& /*Payload*/);

UCLASS()
class A302_API UVoiceNetworkClient : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UObject* OuterObj);
    
    void Connect(const FString& URL);
    void Disconnect();
    bool IsConnected() const;

    void SendVoiceData(const TArray<uint8>& Payload, const FString& RoomCode, const FString& Mode, const FString& SpeakerName);
    
    FOnVoiceBinaryPacketReceived OnBinaryPacketReceived;

private:
    void HandleBinaryMessage(const TArray<uint8>& BinaryData);

    void SendLeavePacket();

    UPROPERTY()
    class UGameNetworkSubsystem* GameNetworkSubsystem = nullptr;
};
