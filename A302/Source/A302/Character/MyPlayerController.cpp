// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/MyPlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

AMyPlayerController::AMyPlayerController()
{

}

void AMyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("[PC] BeginPlay. DefaultMappingContext=%s Priority=%d"),
    *GetNameSafe(DefaultMappingContext), MappingPriority);

	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] LocalPlayer OK"));
		if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			UE_LOG(LogTemp, Warning, TEXT("[PC] EnhancedInputLocalPlayerSubsystem OK"));
			if (DefaultMappingContext)
			{
				Subsys->AddMappingContext(DefaultMappingContext, MappingPriority);
				UE_LOG(LogTemp, Warning, TEXT("[PC] AddMappingContext DONE"));
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
}