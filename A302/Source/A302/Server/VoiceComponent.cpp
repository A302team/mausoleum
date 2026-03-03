// Fill out your copyright notice in the Description page of Project Settings.

#include "Server/VoiceComponent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#if !UE_SERVER
#include "VoiceChat.h"
#endif

UVoiceComponent::UVoiceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UVoiceComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoInitialize)
	{
		InitializeVoice();
	}
}

void UVoiceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopTalking();
	CleanupVoiceUser();
	Super::EndPlay(EndPlayReason);
}

bool UVoiceComponent::IsLocalOwner() const
{
	const APawn* PawnOwner = Cast<APawn>(GetOwner());
	return PawnOwner && PawnOwner->IsLocallyControlled();
}

FString UVoiceComponent::ResolvePlayerName() const
{
	if (!VoicePlayerName.IsEmpty())
	{
		return VoicePlayerName;
	}

	const APawn* PawnOwner = Cast<APawn>(GetOwner());
	if (PawnOwner && PawnOwner->GetPlayerState())
	{
		return PawnOwner->GetPlayerState()->GetPlayerName();
	}

	return FString::Printf(TEXT("Player_%s"), *GetNameSafe(GetOwner()));
}

void UVoiceComponent::InitializeVoice()
{
#if UE_SERVER
	return;
#else
	if (!IsLocalOwner())
	{
		return;
	}

	if (!VoiceChat)
	{
		VoiceChat = IVoiceChat::Get();
		if (!VoiceChat)
		{
			UE_LOG(LogTemp, Error, TEXT("[Voice] IVoiceChat unavailable. EOSVoiceChat plugin/config 확인 필요."));
			return;
		}
	}

	if (!VoiceChat->IsInitialized())
	{
		if (!VoiceChat->Initialize())
		{
			UE_LOG(LogTemp, Error, TEXT("[Voice] Initialize failed"));
			return;
		}
	}

	if (!VoiceUser)
	{
		VoiceUser = VoiceChat->CreateUser();
		if (!VoiceUser)
		{
			UE_LOG(LogTemp, Error, TEXT("[Voice] CreateUser failed"));
			return;
		}
	}

	if (VoiceChat->IsConnected())
	{
		bConnected = true;
		if (bAutoLogin && !bLoggedIn)
		{
			LoginVoice();
		}
		return;
	}

	if (VoiceChat->IsConnecting())
	{
		return;
	}

	VoiceChat->Connect(FOnVoiceChatConnectCompleteDelegate::CreateUObject(this, &UVoiceComponent::HandleConnectComplete));
#endif
}

void UVoiceComponent::HandleConnectComplete(const FVoiceChatResult& Result)
{
#if UE_SERVER
	return;
#else
	if (!Result.IsSuccess())
	{
		UE_LOG(LogTemp, Error, TEXT("[Voice] Connect failed: %s"), *LexToString(Result));
		return;
	}

	bConnected = true;
	UE_LOG(LogTemp, Log, TEXT("[Voice] Connected"));

	if (bAutoLogin && !bLoggedIn)
	{
		LoginVoice();
	}
#endif
}

void UVoiceComponent::LoginVoice()
{
#if UE_SERVER
	return;
#else
	if (!VoiceUser)
	{
		InitializeVoice();
		if (!VoiceUser) return;
	}

	if (!bConnected)
	{
		InitializeVoice();
		return;
	}

	if (VoiceUser->IsLoggedIn())
	{
		bLoggedIn = true;
		if (bAutoJoinChannel && !bChannelJoined)
		{
			JoinVoiceChannel();
		}
		return;
	}

	const FString PlayerName = ResolvePlayerName();
	FString Credentials = VoiceLoginCredentials;
	if (Credentials.IsEmpty())
	{
		Credentials = VoiceUser->InsecureGetLoginToken(PlayerName);
	}

	const FPlatformUserId PlatformId = FPlatformUserId::CreateFromInternalId(0);
	VoiceUser->Login(PlatformId, PlayerName, Credentials, FOnVoiceChatLoginCompleteDelegate::CreateUObject(this, &UVoiceComponent::HandleLoginComplete));
#endif
}

void UVoiceComponent::HandleLoginComplete(const FString& PlayerName, const FVoiceChatResult& Result)
{
#if UE_SERVER
	return;
#else
	if (!Result.IsSuccess())
	{
		UE_LOG(LogTemp, Error, TEXT("[Voice] Login failed (%s): %s"), *PlayerName, *LexToString(Result));
		return;
	}

	bLoggedIn = true;
	UE_LOG(LogTemp, Log, TEXT("[Voice] Logged in as %s"), *PlayerName);

	if (bAutoJoinChannel && !bChannelJoined)
	{
		JoinVoiceChannel();
	}
#endif
}

void UVoiceComponent::JoinVoiceChannel()
{
#if UE_SERVER
	return;
#else
	if (!VoiceUser || !bLoggedIn)
	{
		return;
	}

	const FString ChannelName = VoiceChannelName.IsEmpty() ? TEXT("Global") : VoiceChannelName;
	FString Credentials = VoiceChannelCredentials;
	if (Credentials.IsEmpty())
	{
		Credentials = VoiceUser->InsecureGetJoinToken(ChannelName, EVoiceChatChannelType::NonPositional);
	}

	VoiceUser->JoinChannel(
		ChannelName,
		Credentials,
		EVoiceChatChannelType::NonPositional,
		FOnVoiceChatChannelJoinCompleteDelegate::CreateUObject(this, &UVoiceComponent::HandleJoinComplete));
#endif
}

void UVoiceComponent::HandleJoinComplete(const FString& ChannelName, const FVoiceChatResult& Result)
{
#if UE_SERVER
	return;
#else
	if (!Result.IsSuccess())
	{
		UE_LOG(LogTemp, Error, TEXT("[Voice] JoinChannel failed (%s): %s"), *ChannelName, *LexToString(Result));
		return;
	}

	bChannelJoined = true;
	VoiceChannelName = ChannelName;
	VoiceUser->SetAudioInputDeviceMuted(true);
	VoiceUser->TransmitToNoChannels();
	UE_LOG(LogTemp, Log, TEXT("[Voice] Joined channel: %s"), *ChannelName);
#endif
}

void UVoiceComponent::StartTalking()
{
#if UE_SERVER
	return;
#else
	if (!bChannelJoined)
	{
		InitializeVoice();
		if (!bChannelJoined && bLoggedIn)
		{
			JoinVoiceChannel();
		}
		return;
	}

	if (!VoiceUser) return;

	VoiceUser->SetAudioInputDeviceMuted(false);
	TSet<FString> TransmitChannels;
	TransmitChannels.Add(VoiceChannelName);
	VoiceUser->TransmitToSpecificChannels(TransmitChannels);
#endif
}

void UVoiceComponent::StopTalking()
{
#if UE_SERVER
	return;
#else
	if (!VoiceUser || !bLoggedIn) return;

	VoiceUser->SetAudioInputDeviceMuted(true);
	VoiceUser->TransmitToNoChannels();
#endif
}

void UVoiceComponent::CleanupVoiceUser()
{
#if UE_SERVER
	return;
#else
	if (!VoiceUser || !VoiceChat)
	{
		return;
	}

	const FString ChannelName = VoiceChannelName;
	if (bChannelJoined && !ChannelName.IsEmpty())
	{
		VoiceUser->LeaveChannel(ChannelName,
			FOnVoiceChatChannelLeaveCompleteDelegate::CreateLambda([](const FString&, const FVoiceChatResult&){}));
	}

	if (VoiceUser->IsLoggedIn())
	{
		VoiceUser->Logout(FOnVoiceChatLogoutCompleteDelegate::CreateLambda([](const FString&, const FVoiceChatResult&){}));
	}

	VoiceChat->ReleaseUser(VoiceUser);
	VoiceUser = nullptr;
	bConnected = false;
	bLoggedIn = false;
	bChannelJoined = false;
#endif
};

