#include "Character/Components/PrivateVoiceChatComponent.h"

#include "Dom/JsonObject.h"
#include "Network/WebSocketManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Voice/Strategy/DistanceVoiceChatStrategy.h"
#include "Voice/Strategy/LobbyVoiceChatStrategy.h"
#include "Voice/Strategy/VoiceChatStrategyBase.h"

UPrivateVoiceChatComponent::UPrivateVoiceChatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPrivateVoiceChatComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!WebSocketManager)
    {
        WebSocketManager = NewObject<UWebSocketManager>(this);
        if (WebSocketManager)
        {
            WebSocketManager->OnMessageReceived.AddDynamic(this, &UPrivateVoiceChatComponent::HandleServerMessage);
        }
    }

    if (!LobbyStrategy)
    {
        LobbyStrategy = NewObject<ULobbyVoiceChatStrategy>(this);
    }

    if (!DistanceStrategy)
    {
        DistanceStrategy = NewObject<UDistanceVoiceChatStrategy>(this);
    }

    if (DistanceStrategy)
    {
        DistanceStrategy->SetHearingDistance(DefaultInGameHearingDistance);
    }

    if (InitialVoiceMode == EVoiceChatMode::Lobby)
    {
        SetLobbyMode();
    }
    else
    {
        SetDistanceMode(DefaultInGameHearingDistance);
    }

    if (bAutoConnectOnBeginPlay)
    {
        ConnectToVoiceServer();
    }
}

void UPrivateVoiceChatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    DisconnectFromVoiceServer();
    Super::EndPlay(EndPlayReason);
}

void UPrivateVoiceChatComponent::ConnectToVoiceServer()
{
    if (!WebSocketManager || VoiceServerUrl.IsEmpty())
    {
        return;
    }

    if (!WebSocketManager->IsConnected())
    {
        WebSocketManager->Connect(VoiceServerUrl);
    }
}

void UPrivateVoiceChatComponent::DisconnectFromVoiceServer()
{
    if (!WebSocketManager)
    {
        return;
    }

    WebSocketManager->Disconnect();
}

void UPrivateVoiceChatComponent::SetLobbyMode()
{
    ApplyStrategy(LobbyStrategy);
}

void UPrivateVoiceChatComponent::SetDistanceMode(float NewDistance)
{
    if (DistanceStrategy && NewDistance > 0.f)
    {
        DistanceStrategy->SetHearingDistance(NewDistance);
    }

    ApplyStrategy(DistanceStrategy);
}

void UPrivateVoiceChatComponent::SetRoomId(const FString& InRoomId)
{
    RoomId = InRoomId;
}

EVoiceChatMode UPrivateVoiceChatComponent::GetCurrentMode() const
{
    if (!ActiveStrategy)
    {
        return EVoiceChatMode::Lobby;
    }

    return ActiveStrategy->GetMode();
}

bool UPrivateVoiceChatComponent::CanHearActor(const AActor* SpeakerActor) const
{
    if (!SpeakerActor || !ActiveStrategy)
    {
        return false;
    }

    const UPrivateVoiceChatComponent* SpeakerComp = SpeakerActor->FindComponentByClass<UPrivateVoiceChatComponent>();
    if (!SpeakerComp)
    {
        return false;
    }

    return ActiveStrategy->CanReceiveVoice(this, SpeakerComp);
}

void UPrivateVoiceChatComponent::SendVoicePayload(const FString& EncodedPayload)
{
    if (!WebSocketManager || !WebSocketManager->IsConnected() || !ActiveStrategy)
    {
        return;
    }

    const FString ModeString = (GetCurrentMode() == EVoiceChatMode::Lobby) ? TEXT("lobby") : TEXT("distance");

    TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    JsonObject->SetStringField(TEXT("type"), TEXT("voice_data"));
    JsonObject->SetStringField(TEXT("mode"), ModeString);
    JsonObject->SetStringField(TEXT("roomId"), RoomId);
    JsonObject->SetStringField(TEXT("speaker"), GetNameSafe(GetOwner()));
    JsonObject->SetStringField(TEXT("payload"), EncodedPayload);

    FString OutMessage;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutMessage);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    WebSocketManager->SendMessage(OutMessage);
}

void UPrivateVoiceChatComponent::HandleServerMessage(const FString& Message)
{
    OnVoiceServerMessage.Broadcast(Message);
}

void UPrivateVoiceChatComponent::ApplyStrategy(UVoiceChatStrategyBase* NewStrategy)
{
    if (!NewStrategy)
    {
        return;
    }

    ActiveStrategy = NewStrategy;
    OnVoiceModeChanged.Broadcast(ActiveStrategy->GetMode());
}
