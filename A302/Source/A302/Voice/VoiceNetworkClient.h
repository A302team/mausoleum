#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "VoiceNetworkClient.generated.h"

class UWebSocketManager;

DECLARE_DELEGATE_OneParam(FOnVoicePacketReceived, const FString& /*JsonMessage*/);

UCLASS()
class A302_API UVoiceNetworkClient : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(UObject* OuterObj);
    
    void Connect(const FString& URL);
    void Disconnect();
    bool IsConnected() const;

    void SendVoiceData(const FString& Payload, const FString& RoomCode, const FString& Mode, const FString& SpeakerName);
    
    FOnVoicePacketReceived OnPacketReceived;

private:
    UFUNCTION()
    void HandleRawMessage(const FString& Message);

    UPROPERTY()
    TObjectPtr<UWebSocketManager> WebSocketManager = nullptr;
};
