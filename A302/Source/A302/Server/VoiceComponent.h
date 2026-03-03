// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VoiceComponent.generated.h"

class IVoiceChat;
class IVoiceChatUser;
struct FVoiceChatResult;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class A302_API UVoiceComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UVoiceComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// EOS Voice login target player name (ProductUserId or account name string)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice|EOS")
	FString VoicePlayerName;

	// EOS Voice login token. Empty면 InsecureGetLoginToken() 사용(개발용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice|EOS")
	FString VoiceLoginCredentials;

	// 참여할 음성 채널 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice|EOS")
	FString VoiceChannelName = TEXT("Global");

	// 채널 토큰. Empty면 InsecureGetJoinToken() 사용(개발용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice|EOS")
	FString VoiceChannelCredentials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice|EOS")
	bool bAutoInitialize = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice|EOS")
	bool bAutoLogin = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Voice|EOS")
	bool bAutoJoinChannel = true;

	UFUNCTION(BlueprintCallable, Category = "Voice|EOS")
	void InitializeVoice();

	UFUNCTION(BlueprintCallable, Category = "Voice|EOS")
	void LoginVoice();

	UFUNCTION(BlueprintCallable, Category = "Voice|EOS")
	void JoinVoiceChannel();

	UFUNCTION(BlueprintCallable, Category = "Voice|EOS")
	void StartTalking();

	UFUNCTION(BlueprintCallable, Category = "Voice|EOS")
	void StopTalking();

	UFUNCTION(BlueprintPure, Category = "Voice|EOS")
	bool IsVoiceReady() const { return bChannelJoined; }

private:
	bool IsLocalOwner() const;
	FString ResolvePlayerName() const;
	void HandleConnectComplete(const FVoiceChatResult& Result);
	void HandleLoginComplete(const FString& PlayerName, const FVoiceChatResult& Result);
	void HandleJoinComplete(const FString& ChannelName, const FVoiceChatResult& Result);
	void CleanupVoiceUser();

	IVoiceChat* VoiceChat = nullptr;
	IVoiceChatUser* VoiceUser = nullptr;

	bool bConnected = false;
	bool bLoggedIn = false;
	bool bChannelJoined = false;
};
