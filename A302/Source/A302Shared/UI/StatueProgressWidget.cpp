// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/StatueProgressWidget.h"
#include "Components/ProgressBar.h"
#include "Object/StatueInteractable.h"
#include "GameFramework/Character.h"
#include "Character/Components/Interaction/InteractComponent.h"

void UStatueProgressWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);
	TryBindToInteractComponent();
}

void UStatueProgressWidget::NativeDestruct()
{
	if (CachedInteractComponent)
	{
		CachedInteractComponent->OnHoldInteractionStarted.RemoveDynamic(this, &UStatueProgressWidget::StartTracking);
		CachedInteractComponent->OnHoldInteractionEnded.RemoveDynamic(this, &UStatueProgressWidget::StopTracking);
	}
	Super::NativeDestruct();
}

bool UStatueProgressWidget::TryBindToInteractComponent()
{
	if (CachedInteractComponent) return true;

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ACharacter* Character = Cast<ACharacter>(PC->GetPawn()))
		{
			CachedInteractComponent = Character->FindComponentByClass<UInteractComponent>();
			if (CachedInteractComponent)
			{
				CachedInteractComponent->OnHoldInteractionStarted.AddDynamic(this, &UStatueProgressWidget::StartTracking);
				CachedInteractComponent->OnHoldInteractionEnded.AddDynamic(this, &UStatueProgressWidget::StopTracking);
				return true;
			}
		}
	}
	return false;
}

void UStatueProgressWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 게임 초기에 캐릭터가 아직 빙의되지 않았을 수 있으므로 틱에서 바인딩 재시도
	if (!CachedInteractComponent)
	{
		TryBindToInteractComponent();
	}

	if (CurrentStatue && ProgressBar_Progress)
	{
		float Ratio = 0.0f;
		if (CurrentStatue->MaxProgress > 0.0f)
		{
			Ratio = FMath::Clamp(CurrentStatue->CurrentProgress / CurrentStatue->MaxProgress, 0.0f, 1.0f);
		}
		ProgressBar_Progress->SetPercent(Ratio);
	}
}

void UStatueProgressWidget::StartTracking(AActor* TargetActor)
{
	CurrentStatue = Cast<AStatueInteractable>(TargetActor);
	if (CurrentStatue)
	{
		SetVisibility(ESlateVisibility::HitTestInvisible); // 화면 정중앙에 띄우고 입력을 먹지 않게 설정
	}
}

void UStatueProgressWidget::StopTracking()
{
	CurrentStatue = nullptr;
	SetVisibility(ESlateVisibility::Collapsed);
}
