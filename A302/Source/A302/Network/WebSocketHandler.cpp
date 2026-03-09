#include "Network/WebSocketHandler.h"
#include "WebSocketsModule.h"

void UWebSocketHandler::Connect(const FString& URL)
{
    if (!FModuleManager::Get().IsModuleLoaded("WebSockets"))
    {
        FModuleManager::Get().LoadModule("WebSockets");
    }

    if(WebSocket.IsValid() && WebSocket->IsConnected())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Network/WebSocketHandler] WebSocket is already connected."));
        return;
    }

    FWebSocketsModule* Module = &FWebSocketsModule::Get();
    if (!Module)
    {
        UE_LOG(LogTemp, Error, TEXT("[Network/WebSocketHandler] WebSockets Module load failed!"));
        return;
    }

    WebSocket = FWebSocketsModule::Get().CreateWebSocket(URL, TEXT(""));

    TWeakObjectPtr<UWebSocketHandler> WeakThis(this);

    WebSocket->OnConnected().AddLambda([WeakThis, URL]() {
        if (WeakThis.IsValid())
        {
            UE_LOG(LogTemp, Log, TEXT("[Network/WebSocketHandler] Connected to: %s"), *URL);
        }
    });

    WebSocket->OnMessage().AddLambda([WeakThis](const FString& Message) {
        if (WeakThis.IsValid())
        {
            WeakThis->OnMessageReceived.Broadcast(Message);
        }
    });

    WebSocket->OnRawMessage().AddLambda([WeakThis](const void* Data, SIZE_T Size, SIZE_T BytesRemaining) {
        if (WeakThis.IsValid() && Size > 0)
        {
            TArray<uint8> UncompressedData;
            UncompressedData.Append((const uint8*)Data, Size);
            WeakThis->OnBinaryMessageReceived.Broadcast(UncompressedData);
        }
    });

    WebSocket->OnClosed().AddLambda([WeakThis](int32 Code, const FString& Reason, bool bWasClean) {
        if (WeakThis.IsValid())
        {
            UE_LOG(LogTemp, Log, TEXT("[Network/WebSocketHandler] Disconnect: Code=%d Reason=%s"), Code, *Reason);
        }
    });

    WebSocket->OnConnectionError().AddLambda([WeakThis](const FString& Error) {
        if (WeakThis.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("[Network/WebSocketHandler] Connection Error: %s"), *Error);
        }
    });

    WebSocket->Connect();
}

void UWebSocketHandler::SendMessage(const FString& Message)
{
    if(WebSocket.IsValid() && WebSocket->IsConnected())
    {
        WebSocket->Send(Message);
    }
}

void UWebSocketHandler::SendBinaryMessage(const TArray<uint8>& MessageData)
{
    if(WebSocket.IsValid() && WebSocket->IsConnected())
    {
        WebSocket->Send(MessageData.GetData(), MessageData.Num(), true);
    }
}

void UWebSocketHandler::Disconnect()
{
    if(WebSocket.IsValid())
    {
        UE_LOG(LogTemp, Log, TEXT("[Network/WebSocketHandler] Disconnecting WebSocket..."));
        WebSocket->Close();
    }
}

bool UWebSocketHandler::IsConnected() const
{
    return WebSocket.IsValid() && WebSocket->IsConnected();
}
