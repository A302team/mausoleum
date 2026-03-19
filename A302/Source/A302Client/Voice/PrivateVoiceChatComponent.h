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

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302CLIENT_API UPrivateVoiceChatComponent : public UActorComponent
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

    UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Voice|Server")
    FString VoiceServerUrl = TEXT("[IP_ADDRESS]");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice|Room")
    FString roomCode;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice|Mode", meta=(ClampMin="100.0"))
    float DefaultInGameHearingDistance = 1800.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice|Server")
    bool bAutoConnectOnBeginPlay = true;

    UPROPERTY(BlueprintAssignable, Category = "Voice")
    FOnVoiceModeChanged OnVoiceModeChanged;

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

    // 내 플레이어 이름을 가져오는 헬퍼 (PlayerState → GameInstance 순서로 탐색)
    FString GetMyPlayerName() const;
    FString ResolveCurrentRoomCode() const;

    // 송신 콜백
    void OnVoiceDataCaptured(const TArray<uint8>& VoiceData);
    void OnNetworkBinaryMessageReceived(const FString& RoomCode, const FString& SpeakerName, const TArray<uint8>& VoiceData);

    // 컴포넌트 생성 시점에 로컬 컨트롤 유무 저장 (종속성 해제 시 IsLocallyControlled 신뢰성 저하 대비)
    bool bIsInitializedAsLocal = false;

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

    UPROPERTY()
    TMap<FString, TObjectPtr<class UVoiceCodec>> SpeakerCodecs;
};
