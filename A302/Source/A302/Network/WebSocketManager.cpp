// Fill out your copyright notice in the Description page of Project Settings.


#include "Network/WebSocketManager.h"
#include "WebSocketsModule.h"
#include "WebSocketManager.h"

void UWebSocketManager::Connect(const FString &URL)
{
    if (!FModuleManager::Get().IsModuleLoaded("WebSockets"))
    {
        FModuleManager::Get().LoadModule("WebSockets");
    }

    if(WebSocket.IsValid() && WebSocket->IsConnected())
    {
        UE_LOG(LogTemp, Warning, TEXT("[Network/WebSocket] WebSocket is already connected."));
        return;
    }

    FWebSocketsModule* Module = &FWebSocketsModule::Get();
    if (!Module)
    {
        UE_LOG(LogTemp, Error, TEXT("[Network/WebSocket] WebSockets 모듈 로드 실패!"));
        return;
    }

    WebSocket = FWebSocketsModule::Get().CreateWebSocket(URL, TEXT("ws"));

    WebSocket -> OnConnected().AddLambda([this]() {
        UE_LOG(LogTemp, Log, TEXT("[Network/WebSocket] WebSocket connected."));
    });

    WebSocket -> OnMessage().AddLambda([this](const FString& Message) {
        // UE_LOG(LogTemp, Log, TEXT("[Network/WebSocket] Received : %s"), *Message);
        OnMessageReceived.Broadcast(Message);
    });

    WebSocket -> OnClosed().AddLambda([](int32 Code, const FString& Reason, bool bWasClean) {
        UE_LOG(LogTemp, Log, TEXT("[Network/WebSocket] Disconnect"));
    });

    WebSocket -> OnConnectionError().AddLambda([](const FString& Error) {
        UE_LOG(LogTemp, Log, TEXT("[Network/WebSocket] Error Connection : %s"), *Error);
    });

    WebSocket -> Connect();

}

void UWebSocketManager::SendMessage(const FString &Message)
{
    if(WebSocket.IsValid() && WebSocket->IsConnected())
    {
        UE_LOG(LogTemp, Log, TEXT("[Network/WebSocket] Send Message"));
        WebSocket -> Send(Message);
    }
}

void UWebSocketManager::Disconnect()
{
    if(WebSocket.IsValid())
    {
        UE_LOG(LogTemp, Log, TEXT("[Network/WebSocket] Disconnect"));
        WebSocket->Close();
    }
}

bool UWebSocketManager::IsConnected() const
{
    return WebSocket.IsValid() && WebSocket -> IsConnected();
}
