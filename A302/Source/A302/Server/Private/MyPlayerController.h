// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class A302_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AMyPlayerController();
protected:
	virtual void BeginPlay() override;

public:
	// Function to send a message to the server
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMessage(const FString& Message);

	UPROPERTY(ReplicatedUsing = OnRep_ServerResponse)
	FString ServerResponse;
	
	UFUNCTION()
	void OnRep_ServerResponse();

	// 네트워크 동기화 변수를 등록하기 위한 함수
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
