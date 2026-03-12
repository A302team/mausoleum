#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameNetworkSubsystem.generated.h"

UENUM(BlueprintType)
enum class EAddress : uint8
{
	LOCAL,
	SERVER
};

inline FString GetServerAddress(EAddress Type)
{
	return Type == EAddress::LOCAL ? TEXT("127.0.0.1") : TEXT("43.201.83.68");
}

UENUM(BlueprintType)
enum class EProtocolType : uint8
{
    WebSocket UMETA(DisplayName = "WebSocket (TCP)"),
    UDP UMETA(DisplayName = "UDP/RUDP")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPacketReceived, const FString&, Message);

// C++ 전용 델리게이트 (바이너리 패킷/Protobuf 등 수신용)
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBinaryPacketReceived, const TArray<uint8>&);

namespace PORT
{
	static constexpr int32 LOBBY_PORT = 8001;
	static constexpr int32 VOICE_PORT = 48100;

}

/**
 * 전역 네트워크 상태를 관리하는 서브시스템 (GameInstance 종속)
 * DefaultEngine.ini 등에서 [/Script/A302.GameNetworkSubsystem] 섹션으로 설정 가능
 */
UCLASS(Config=Engine)
class A302_API UGameNetworkSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 서버 주소 설정 (DefaultEngine.ini 등에서 오버라이드 가능)
	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Network|Config")
	FString ServerIP = GetServerAddress(EAddress::LOCAL);

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Network|Config")
	int32 LobbyPort = PORT::LOBBY_PORT;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category = "Network|Config")
	int32 VoicePort = PORT::VOICE_PORT;

	UFUNCTION(BlueprintPure, Category = "Network")
	FString GetCleanServerIP() const
	{
		FString CleanIP = ServerIP;
		CleanIP.ReplaceInline(TEXT("http://"), TEXT(""));
		CleanIP.ReplaceInline(TEXT("https://"), TEXT(""));
		CleanIP.ReplaceInline(TEXT("ws://"), TEXT(""));
		CleanIP.ReplaceInline(TEXT("wss://"), TEXT(""));
        CleanIP.TrimStartAndEndInline();
		//
		return CleanIP;
	}

	UFUNCTION(BlueprintPure, Category = "Network")
	FString GetLobbyURL() const { return GetURL(EProtocolType::WebSocket); }

	UFUNCTION(BlueprintPure, Category = "Network")
	FString GetVoiceURL() const { return GetURL(EProtocolType::UDP); }

	UFUNCTION(BlueprintPure, Category = "Network")
	FString GetURL(EProtocolType Protocol) const
	{
		const FString CleanIP = GetCleanServerIP();
		if (Protocol == EProtocolType::WebSocket)
		{
			return FString::Printf(TEXT("ws://%s:%d"), *CleanIP, LobbyPort);
		}
		return FString::Printf(TEXT("%s:%d"), *CleanIP, VoicePort);
	}

	// 연결 관리
	UFUNCTION(BlueprintCallable, Category = "Network")
	void Connect(EProtocolType Protocol, const FString& URL);

	UFUNCTION(BlueprintCallable, Category = "Network")
	void Disconnect(EProtocolType Protocol);

	UFUNCTION(BlueprintCallable, Category = "Network")
	bool IsConnected(EProtocolType Protocol) const;

	// 패킷 통신
	UFUNCTION(BlueprintCallable, Category = "Network")
	void SendPacket(EProtocolType Protocol, const FString& Payload);

	void SendBinaryPacket(EProtocolType Protocol, const TArray<uint8>& Payload);

	// 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Network")
	FOnPacketReceived OnPacketReceived;

	FOnBinaryPacketReceived OnBinaryPacketReceived;

private:
	UPROPERTY()
	class UWebSocketHandler* WebSocketHandler;

	UPROPERTY()
	class UUDPHandler* UDPHandler;

	UFUNCTION()
	void HandleWebSocketMessage(const FString& Message);

	void HandleWebSocketBinaryMessage(const TArray<uint8>& MessageData);

	UFUNCTION()
	void HandleUDPMessage(const FString& Message);

	void HandleUDPBinaryMessage(const TArray<uint8>& MessageData);
};
