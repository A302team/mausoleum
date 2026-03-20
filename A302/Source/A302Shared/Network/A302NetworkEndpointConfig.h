#pragma once

#include "CoreMinimal.h"

enum class EA302AddressPreset : uint8
{
	Local,
	PublicEc2
};

class A302SHARED_API FA302NetworkEndpointConfig
{
public:
	// Change these presets in code when switching environments.
	static constexpr EA302AddressPreset LobbyAddressPreset = EA302AddressPreset::PublicEc2;
	static constexpr EA302AddressPreset BackendAddressPreset = EA302AddressPreset::PublicEc2;
	static constexpr EA302AddressPreset GameServerAddressPreset = EA302AddressPreset::PublicEc2;

	static constexpr int32 LobbyPort = 8001;
	static constexpr int32 VoicePort = 48100;
	static constexpr int32 GameServerPort = 47777;

	static FString GetLobbyHost();
	static FString GetBackendHost();
	static FString GetGameServerHost();

	static FString GetLobbyWebSocketURL();
	static FString GetBackendWebSocketURL();
	static FString GetVoiceAddress();
	static FString GetGameServerAddress();

private:
	static FString ResolveHost(EA302AddressPreset Preset);
	static FString BuildWebSocketURL(const FString& Host, int32 Port);
	static FString BuildAddress(const FString& Host, int32 Port);
};
