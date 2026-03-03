#include "Character/MyPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"   // CreateWidget / UUserWidget

AMyPlayerController::AMyPlayerController()
{
}

void AMyPlayerController::BeginPlay()
{
    Super::BeginPlay();

    // -------- Enhanced Input Mapping (에러 로그는 유지) --------
    if (ULocalPlayer* LP = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            if (DefaultMappingContext)
            {
                Subsys->AddMappingContext(DefaultMappingContext, MappingPriority);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("[PC] DefaultMappingContext is NULL (BP에서 IMC 연결 안됨)"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[PC] EnhancedInputLocalPlayerSubsystem NULL"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[PC] LocalPlayer NULL"));
    }

    // -------- UI: QuickSlotBar 생성/표시 --------
    if (QuickSlotBarClass)
    {
        QuickSlotBarWidget = CreateWidget<UUserWidget>(this, QuickSlotBarClass);
        if (QuickSlotBarWidget)
        {
            QuickSlotBarWidget->AddToViewport();
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[PC] QuickSlotBarClass is NULL (BP에서 WBP_QuickSlotBar 연결 안됨)"));
    }
}