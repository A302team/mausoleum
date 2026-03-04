// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

/**
 Player Controller
 IMC 활성화
 IMC를 Uproperty로 들고 있음, beginPlay에서 property를 호출하여 함수 바인딩
 * 
 */

class UInputMappingContext;
class UUserWidget;
class UTextBlock;
class UImage;
class UTexture2D;

UCLASS()
class A302_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMyPlayerController();
	bool UpdateQuickSlotItemName(int32 SlotIndex, const FText& ItemName);
	bool UpdateQuickSlotItemVisual(int32 SlotIndex, const FText& ItemName, UTexture2D* ItemIcon);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category="UI")
    TSubclassOf<UUserWidget> QuickSlotBarClass;

    // 런타임에 생성된 위젯 인스턴스(화면에 띄운 객체)
    UPROPERTY()
    TObjectPtr<UUserWidget> QuickSlotBarWidget;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	int32 MappingPriority = 0;

private:
	UUserWidget* FindQuickSlotWidget(int32 SlotIndex) const;
	class UTextBlock* FindQuickSlotItemNameText(int32 SlotIndex) const;
	class UImage* FindQuickSlotItemIconImage(int32 SlotIndex) const;
	void InitializeQuickSlotVisualState();

};
