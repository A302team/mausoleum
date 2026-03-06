#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Voice/VoiceChatMode.h"
#include "PrivateVoiceChatComponent.generated.h"

class UDistanceVoiceChatStrategy;
class ULobbyVoiceChatStrategy;
class UVoiceChatStrategyBase;
class UVoiceCaptureProcessor;
class UVoiceAudioReceiver;
class UVoiceNetworkClient;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVoiceModeChanged, EVoiceChatMode, NewMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVoiceServerMessage, const FString&, Message);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302_API UPrivateVoiceChatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPrivateVoiceChatComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintCallable, Category = "Voice|Server")
    void ConnectToVoiceServer();

    UFUNCTION(BlueprintCallable, Category = "Voice|Server")
    void DisconnectFromVoiceServer();

    UFUNCTION(BlueprintCallable, Category = "Voice|Mode")
    void SetLobbyMode();

    UFUNCTION(BlueprintCallable, Category = "Voice|Mode")
    void SetDistanceMode(float NewDistance = 0.f);

    UFUNCTION(BlueprintCallable, Category = "Voice|Room")
    void SetRoomCode(const FString& InRoomCode);

    UFUNCTION(BlueprintPure, Category = "Voice|Room")
    const FString& GetRoomCode() const { return roomCode; }

    UFUNCTION(BlueprintPure, Category = "Voice|Mode")
    EVoiceChatMode GetCurrentMode() const;

    UFUNCTION(BlueprintPure, Category = "Voice|Debug")
    bool CanHearActor(const AActor* SpeakerActor) const;

    UFUNCTION(BlueprintCallable, Category = "Voice|Server")
    void SendVoicePayload(const FString& EncodedPayload);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice|Server")
    FString VoiceServerUrl = TEXT("ws://127.0.0.1:9100");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice|Room")
    FString roomCode;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice|Mode", meta=(ClampMin="100.0"))
    float DefaultInGameHearingDistance = 1800.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice|Mode")
    EVoiceChatMode InitialVoiceMode = EVoiceChatMode::Lobby;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice|Server")
    bool bAutoConnectOnBeginPlay = true;

    UPROPERTY(BlueprintAssignable, Category = "Voice")
    FOnVoiceModeChanged OnVoiceModeChanged;

    UPROPERTY(BlueprintAssignable, Category = "Voice")
    FOnVoiceServerMessage OnVoiceServerMessage;

    UFUNCTION(BlueprintCallable, Category = "Voice|Capture")
    void StartMicrophone();

    UFUNCTION(BlueprintCallable, Category = "Voice|Capture")
    void StopMicrophone();

    UFUNCTION(BlueprintCallable, Category = "Voice|Capture")
    void ToggleMicrophone();

    UFUNCTION(BlueprintPure, Category = "Voice|Capture")
    bool IsMicrophoneActive() const;

    UFUNCTION(BlueprintCallable, Category = "Voice|Capture")
    bool ChangeMicrophone(const FString& DeviceName);

private:
    void ApplyStrategy(UVoiceChatStrategyBase* NewStrategy);

    void OnVoiceDataCaptured(const FString& EncodedPayload);
    void OnNetworkMessageReceived(const FString& JsonMessage);

    UPROPERTY()
    TObjectPtr<UVoiceCaptureProcessor> CaptureProcessor = nullptr;

    UPROPERTY()
    TObjectPtr<UVoiceAudioReceiver> AudioReceiver = nullptr;

    UPROPERTY()
    TObjectPtr<UVoiceNetworkClient> NetworkClient = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<ULobbyVoiceChatStrategy> LobbyStrategy = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UDistanceVoiceChatStrategy> DistanceStrategy = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UVoiceChatStrategyBase> ActiveStrategy = nullptr;
};
