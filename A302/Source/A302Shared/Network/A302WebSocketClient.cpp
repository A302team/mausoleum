#include "Network/A302WebSocketClient.h"

#include "IWebSocket.h"
#include "Modules/ModuleManager.h"
#include "WebSocketsModule.h"

void UA302WebSocketClient::Connect(const FString& URL, const FString& LogPrefix)
{
	CurrentLogPrefix = LogPrefix;

	if (!FModuleManager::Get().IsModuleLoaded("WebSockets"))
	{
		FModuleManager::Get().LoadModule("WebSockets");
	}

	if (WebSocket.IsValid() && WebSocket->IsConnected())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s WebSocket is already connected."), *CurrentLogPrefix);
		return;
	}

	FWebSocketsModule& Module = FWebSocketsModule::Get();
	WebSocket = Module.CreateWebSocket(URL, TEXT(""));

	TWeakObjectPtr<UA302WebSocketClient> WeakThis(this);

	WebSocket->OnConnected().AddLambda([WeakThis, URL]()
	{
		if (WeakThis.IsValid())
		{
			UE_LOG(LogTemp, Log, TEXT("%s Connected to: %s"), *WeakThis->CurrentLogPrefix, *URL);
			WeakThis->OnConnected.Broadcast();
		}
	});

	WebSocket->OnMessage().AddLambda([WeakThis](const FString& Message)
	{
		if (WeakThis.IsValid())
		{
			WeakThis->OnMessageReceived.Broadcast(Message);
		}
	});

	WebSocket->OnRawMessage().AddLambda([WeakThis](const void* Data, SIZE_T Size, SIZE_T BytesRemaining)
	{
		if (WeakThis.IsValid() && Size > 0)
		{
			TArray<uint8> UncompressedData;
			UncompressedData.Append(static_cast<const uint8*>(Data), Size);
			WeakThis->OnBinaryMessageReceived.Broadcast(UncompressedData);
		}
	});

	WebSocket->OnClosed().AddLambda([WeakThis](int32 Code, const FString& Reason, bool bWasClean)
	{
		if (WeakThis.IsValid())
		{
			UE_LOG(
				LogTemp,
				Log,
				TEXT("%s Disconnect: Code=%d Reason=%s Clean=%s"),
				*WeakThis->CurrentLogPrefix,
				Code,
				*Reason,
				bWasClean ? TEXT("true") : TEXT("false")
			);
		}
	});

	WebSocket->OnConnectionError().AddLambda([WeakThis](const FString& Error)
	{
		if (WeakThis.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("%s Connection Error: %s"), *WeakThis->CurrentLogPrefix, *Error);
		}
	});

	WebSocket->Connect();
}

void UA302WebSocketClient::Disconnect()
{
	if (WebSocket.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("%s Disconnecting WebSocket..."), *CurrentLogPrefix);
		WebSocket->Close();
		WebSocket.Reset();
	}
}

void UA302WebSocketClient::SendMessage(const FString& Message)
{
	if (WebSocket.IsValid() && WebSocket->IsConnected())
	{
		WebSocket->Send(Message);
	}
}

void UA302WebSocketClient::SendBinaryMessage(const TArray<uint8>& MessageData)
{
	if (WebSocket.IsValid() && WebSocket->IsConnected())
	{
		WebSocket->Send(MessageData.GetData(), MessageData.Num(), true);
	}
}

bool UA302WebSocketClient::IsConnected() const
{
	return WebSocket.IsValid() && WebSocket->IsConnected();
}
