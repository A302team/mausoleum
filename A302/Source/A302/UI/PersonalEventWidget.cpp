// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/PersonalEventWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Character/MyPlayerController.h"

void UPersonalEventWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 확인 버튼 이벤트 바인딩
	if (Btn_Confirm) Btn_Confirm->OnClicked.AddDynamic(this, &UPersonalEventWidget::OnConfirmClicked);
	
	// 취소 버튼 이벤트 바인딩
	if (Btn_Cancel) Btn_Cancel->OnClicked.AddDynamic(this, &UPersonalEventWidget::OnCancelClicked);
}

void UPersonalEventWidget::SetupEventUI(FName InEventID, const FText& EventTitle, const FText& EventDescription, bool bIsCancelable)
{
	CurrentEventID = InEventID;
	if (Text_Title) Text_Title->SetText(EventTitle);
	if (Text_Description) Text_Description->SetText(EventDescription);
	if (Btn_Cancel) Btn_Cancel->SetVisibility(bIsCancelable ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UPersonalEventWidget::OnConfirmClicked()
{
	if (AMyPlayerController* PC = Cast<AMyPlayerController>(GetOwningPlayer()))
	{
		PC->Server_ResolvePersonalEvent(CurrentEventID, true);
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
	// 화면에서 위젯 제거
	RemoveFromParent();
}

void UPersonalEventWidget::OnCancelClicked()
{
	if (AMyPlayerController* PC = Cast<AMyPlayerController>(GetOwningPlayer()))
	{
		PC->Server_ResolvePersonalEvent(CurrentEventID, false);
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
	RemoveFromParent();
}