#include "Network/A302NetworkEndpointConfig.h"

FString FA302NetworkEndpointConfig::GetLobbyHost()
{
	return ResolveHost(LobbyAddressPreset);
}

FString FA302NetworkEndpointConfig::GetBackendHost()
{
	return ResolveHost(BackendAddressPreset);
}

FString FA302NetworkEndpointConfig::GetGameServerHost()
{
	return ResolveHost(GameServerAddressPreset);
}

FString FA302NetworkEndpointConfig::GetLobbyWebSocketURL()
{
	return BuildWebSocketURL(GetLobbyHost(), LobbyPort);
}

FString FA302NetworkEndpointConfig::GetBackendWebSocketURL()
{
	return BuildWebSocketURL(GetBackendHost(), LobbyPort);
}

FString FA302NetworkEndpointConfig::GetVoiceAddress()
{
	return BuildAddress(GetLobbyHost(), VoicePort);
}

FString FA302NetworkEndpointConfig::GetGameServerAddress()
{
	return BuildAddress(GetGameServerHost(), GameServerPort);
}

FString FA302NetworkEndpointConfig::ResolveHost(EA302AddressPreset Preset)
{
	switch (Preset)
	{
	case EA302AddressPreset::PublicEc2:
		return TEXT("43.201.83.68");
	case EA302AddressPreset::Local:
	default:
		return TEXT("127.0.0.1");
	}
}

FString FA302NetworkEndpointConfig::BuildWebSocketURL(const FString& Host, int32 Port)
{
	return FString::Printf(TEXT("ws://%s:%d"), *Host, Port);
}

FString FA302NetworkEndpointConfig::BuildAddress(const FString& Host, int32 Port)
{
	return FString::Printf(TEXT("%s:%d"), *Host, Port);
}
