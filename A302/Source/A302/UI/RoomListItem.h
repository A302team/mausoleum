// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RoomListItem.generated.h"

/**
 *
 */
UCLASS()
class A302_API URoomListItem : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// 방 정보 텍스트
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> Text_RoomInfo;

	// 입장 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> Btn_Join;

	// 방 코드 저장
	FString RoomCode;

	// 방 정보 세팅
	void SetRoomInfo(const FString &InRoomCode, int32 PlayerCount);

	UFUNCTION()
	void OnJoinClicked();

private:
	TObjectPtr<class ALobbyGameMode> LobbyGameMode;
};
