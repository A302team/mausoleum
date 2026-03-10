// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PersonalEventWidget.generated.h"

class UTextBlock;
class UButton;

UCLASS()
class A302_API UPersonalEventWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// 클라이언트가 RPC를 받았을 때 이 함수를 호출하여 화면을 갱신합니다.
	UFUNCTION(BlueprintCallable, Category = "Event UI")
	void SetupEventUI(FName InEventID, const FText& EventTitle, const FText& EventDescription, bool bIsCancelable);

protected:
	virtual void NativeConstruct() override;

	// UMG(블루프린트 위젯)의 텍스트/버튼과 C++ 변수를 연결합니다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Title;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Description;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Confirm;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Btn_Cancel;

	UFUNCTION()
	void OnConfirmClicked();

	UFUNCTION()
	void OnCancelClicked();
	
private:
	FName CurrentEventID;
};
