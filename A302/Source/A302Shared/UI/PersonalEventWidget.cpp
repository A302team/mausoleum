// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/PersonalEventWidget.h"
#include "Character/Components/PlayerEventComponent.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Character/MyPlayerController.h"

void UPersonalEventWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UPersonalEventWidget::SetupEventUI(FName InEventID, const FText& EventTitle, const FText& EventDescription, const TArray<FText>& Choices)
{
	CurrentEventID = InEventID;
	if (Text_Title) Text_Title->SetText(EventTitle);
	if (Text_Description) Text_Description->SetText(EventDescription);
	if (Box_Choices) Box_Choices->ClearChildren();
	for (int32 i = 0; i < Choices.Num(); ++i) AddChoiceButton(i, Choices[i]);
}

void UPersonalEventWidget::OnChoiceButtonClicked(int32 ChoiceIndex)
{
    if (AMyPlayerController* PC = Cast<AMyPlayerController>(GetOwningPlayer()))
    {
        if (UPlayerEventComponent* EventComponent = PC->GetPlayerEventComponent())
        {
            EventComponent->RequestResolvePersonalEvent(CurrentEventID, ChoiceIndex);
        }
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
    }
    RemoveFromParent();
}
