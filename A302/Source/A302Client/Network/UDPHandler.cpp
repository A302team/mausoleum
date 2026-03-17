#include "Network/UDPHandler.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Common/UdpSocketBuilder.h"
#include "Common/UdpSocketReceiver.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "IPAddressAsyncResolve.h"
#include "Serialization/ArrayReader.h"
#include "Async/Async.h"

bool UUDPHandler::ParseUrl(const FString& URL, FString& OutHostName, int32& OutPort) const
{
	FString PortString;
	if (!URL.Split(TEXT(":"), &OutHostName, &PortString))
	{
		UE_LOG(LogTemp, Error, TEXT("[Network/UDPHandler] Invalid UDP URL formatting (missing port): %s"), *URL);
		return false;
	}

	if (!PortString.IsNumeric() || !LexTryParseString(OutPort, *PortString))
	{
		UE_LOG(LogTemp, Error, TEXT("[Network/UDPHandler] Invalid port: %s"), *PortString);
		return false;
	}

	return true;
}

void UUDPHandler::StartSocket(const FString& URL, int32 Port, const TSharedRef<FInternetAddr>& ResolvedAddr)
{
	if (IsConnected())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Network/UDPHandler] Already Connected."));
		return;
	}

	TargetAddr = ResolvedAddr;
	TargetAddr->SetPort(Port);

	// 송수신 겸용 소켓 생성 (포트 0 = OS가 임시 포트 자동 할당)
	UDPSocket = FUdpSocketBuilder(TEXT("UDP_VoiceSocket"))
		.AsReusable()
		.AsNonBlocking()
		.BoundToPort(0)        // 수신을 위해 반드시 바인딩 필요!
		.WithBroadcast()
		.WithReceiveBufferSize(65536);

	if (UDPSocket)
	{
		// 수신 리스너 시작
		SocketReceiver = new FUdpSocketReceiver(
			UDPSocket,
			FTimespan::FromMilliseconds(2),
			TEXT("UDP_VoiceReceiver")
		);
		SocketReceiver->OnDataReceived().BindUObject(this, &UUDPHandler::OnDataReceived);
		SocketReceiver->Start();

		UE_LOG(LogTemp, Log, TEXT("[Network/UDPHandler] UDP Socket Created + Receiver Started. Targeting URL=%s, Resolved=%s"), *URL, *TargetAddr->ToString(true));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Network/UDPHandler] Failed to create UDP Socket."));
	}
}

void UUDPHandler::Connect(const FString& URL)
{
	// 기본 구현 로직 (차후 실제 스펙에 맞게 확장 필요)
	if (IsConnected())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Network/UDPHandler] Already Connected."));
		return;
	}

	FString HostName;
	int32 Port = 0;
	if (!ParseUrl(URL, HostName, Port))
	{
		return;
	}

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		UE_LOG(LogTemp, Error, TEXT("[Network/UDPHandler] Socket subsystem not available."));
		return;
	}

	TSharedRef<FInternetAddr> DirectAddr = SocketSubsystem->CreateInternetAddr();
	bool bIsValid = false;
	DirectAddr->SetIp(*HostName, bIsValid);
	if (!bIsValid)
	{
		UE_LOG(LogTemp, Error, TEXT("[Network/UDPHandler] DNS resolution disabled. Use numeric IP instead of hostname: %s"), *HostName);
		return;
	}

	StartSocket(URL, Port, DirectAddr);
}

void UUDPHandler::Disconnect()
{
	if (SocketReceiver)
	{
		SocketReceiver->Stop();
		delete SocketReceiver;
		SocketReceiver = nullptr;
	}

	if (UDPSocket)
	{
		UDPSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(UDPSocket);
		UDPSocket = nullptr;
		TargetAddr.Reset();
		UE_LOG(LogTemp, Log, TEXT("[Network/UDPHandler] UDP Socket Disconnected."));
	}
}

void UUDPHandler::SendMessage(const FString& Message)
{
	if (!IsConnected()) return;

	FTCHARToUTF8 Convert(*Message);
	TArray<uint8> Data;
	Data.Append((uint8*)Convert.Get(), Convert.Length());

	SendBinaryMessage(Data);
}

void UUDPHandler::SendBinaryMessage(const TArray<uint8>& MessageData)
{
	if (!IsConnected() || !TargetAddr.IsValid()) return;

	int32 BytesSent = 0;
	UDPSocket->SendTo(MessageData.GetData(), MessageData.Num(), BytesSent, *TargetAddr);
}

bool UUDPHandler::IsConnected() const
{
	return UDPSocket != nullptr;
}

void UUDPHandler::OnDataReceived(const FArrayReaderPtr& Data, const FIPv4Endpoint& Sender)
{
	// FUdpSocketReceiver는 백그라운드 스레드에서 실행됨!
	// UObject/Actor 접근은 반드시 Game Thread에서만 가능하므로 전환 필요
	TArray<uint8> ReceivedData;
	ReceivedData.Append(Data->GetData(), Data->Num());

	AsyncTask(ENamedThreads::GameThread, [this, ReceivedData = MoveTemp(ReceivedData)]()
	{
		UE_LOG(LogTemp, Log, TEXT("[Network/UDPHandler] UDP 수신! %d 바이트"), ReceivedData.Num());
		OnBinaryMessageReceived.Broadcast(ReceivedData);
	});
}
