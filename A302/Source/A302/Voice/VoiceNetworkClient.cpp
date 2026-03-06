#include "Voice/VoiceNetworkClient.h"
#include "Network/WebSocketManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"

void UVoiceNetworkClient::Initialize(UObject* OuterObj)
{
    if (!OuterObj) return;

    if (!WebSocketManager)
    {
        WebSocketManager = NewObject<UWebSocketManager>(OuterObj);
        if (WebSocketManager)
        {
            WebSocketManager->OnMessageReceived.AddDynamic(this, &UVoiceNetworkClient::HandleRawMessage);
        }
    }
}

void UVoiceNetworkClient::Connect(const FString& URL)
{
    if (WebSocketManager && !URL.IsEmpty() && !WebSocketManager->IsConnected())
    {
        WebSocketManager->Connect(URL);
    }
}

void UVoiceNetworkClient::Disconnect()
{
    if (WebSocketManager)
    {
        WebSocketManager->Disconnect();
    }
}

bool UVoiceNetworkClient::IsConnected() const
{
    return WebSocketManager && WebSocketManager->IsConnected();
}

void UVoiceNetworkClient::SendVoiceData(const FString& Payload, const FString& RoomCode, const FString& Mode, const FString& SpeakerName)
{
    if (!IsConnected()) return;

    TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    JsonObject->SetStringField(TEXT("type"), TEXT("voice_data"));
    JsonObject->SetStringField(TEXT("mode"), Mode);
    JsonObject->SetStringField(TEXT("roomCode"), RoomCode);
    JsonObject->SetStringField(TEXT("speaker"), SpeakerName);
    JsonObject->SetStringField(TEXT("payload"), Payload);

    FString OutMessage;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutMessage);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    WebSocketManager->SendMessage(OutMessage);
}

void UVoiceNetworkClient::HandleRawMessage(const FString& Message)
{
    OnPacketReceived.ExecuteIfBound(Message);
}
