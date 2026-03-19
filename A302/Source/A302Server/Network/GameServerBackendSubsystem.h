#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Network/A302NetworkEndpointConfig.h"
#include "Network/A302WebSocketClient.h"
#include "GameServerBackendSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnServerBackendPacketReceived, const FString&, Message);

UCLASS()
class A302SERVER_API UGameServerBackendSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Backend")
	bool bAutoConnectOnServerStart = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Backend")
	FString DedicatedServerId = TEXT("A302-Dedicated");

	UFUNCTION(BlueprintPure, Category = "Backend")
	FString GetBackendURL() const;

	UFUNCTION(BlueprintCallable, Category = "Backend")
	void ConnectToBackend();

	UFUNCTION(BlueprintCallable, Category = "Backend")
	void DisconnectFromBackend();

	UFUNCTION(BlueprintCallable, Category = "Backend")
	bool IsConnected() const;

	UFUNCTION(BlueprintCallable, Category = "Backend")
	void SendPacket(const FString& Payload);

	UPROPERTY(BlueprintAssignable, Category = "Backend")
	FOnServerBackendPacketReceived OnPacketReceived;

private:
	bool ShouldOperateAsDedicatedServer() const;

	UFUNCTION()
	void HandleBackendConnected();

	UFUNCTION()
	void HandleBackendMessage(const FString& Message);

	void RegisterDedicatedIdentity();

	UPROPERTY()
	TObjectPtr<UA302WebSocketClient> SharedClient;

	bool bDedicatedRegistrationSent = false;
};
