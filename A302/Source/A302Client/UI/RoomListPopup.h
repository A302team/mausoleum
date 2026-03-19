// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameMode/A302GameInstance.h"
#include "RoomListPopup.generated.h"

/**
 *
 */
UCLASS()
class A302CLIENT_API URoomListPopup : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// 방 목록 스크롤 박스
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UScrollBox> ScrollBox_RoomList;

	// 새로고침 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> Btn_Refresh;

	// 닫기 버튼
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UButton> Btn_Close;

	// 방 목록 아이템 클래스
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<class URoomListItem> RoomListItemClass;

	// 방 목록 업데이트
	UFUNCTION()
	void UpdateRoomList(const TArray<FRoomInfo> &RoomList);

	UFUNCTION()
	void OnRefreshClicked();

	UFUNCTION()
	void OnCloseClicked();

private:
	TObjectPtr<class UA302GameInstance> GI;
};
