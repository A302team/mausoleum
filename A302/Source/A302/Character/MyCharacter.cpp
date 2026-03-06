// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/MyCharacter.h"
#include "Character/Components/InteractComponent.h"
#include "Character/Components/KnifeAutoTestComponent.h"
#include "Character/Components/QuickSlotComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "InputAction.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyInput, Log, All);

AMyCharacter::AMyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	
	InteractionComponent = CreateDefaultSubobject<UInteractComponent>(TEXT("InteractionComponent"));
	QuickSlotComponent = CreateDefaultSubobject<UQuickSlotComponent>(TEXT("QuickSlotComponent"));
	KnifeAutoTestComponent = CreateDefaultSubobject<UKnifeAutoTestComponent>(
		TEXT("KnifeAutoTestComponent")
	);
}

void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (KnifeAutoTestComponent)
	{
		KnifeAutoTestComponent->StartAutoKnifeTest();
	}
}

void AMyCharacter::SetQTEInputMode(bool bIsQTE)
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (bIsQTE)
			{
				// 1. 기본 조작 차단 및 QTE 조작 활성화
				if (IMC_Default) Subsystem->RemoveMappingContext(IMC_Default);
				if (IMC_QTE) Subsystem->AddMappingContext(IMC_QTE, 1);

				// 2. 물리 관성 완벽 정지
				if (GetCharacterMovement())
				{
					GetCharacterMovement()->StopMovementImmediately();
					GetCharacterMovement()->SetMovementMode(MOVE_None);
				}
				UE_LOG(LogMyInput, Log, TEXT("[Input] QTE 전용 입력 모드로 전환 완료"));
			}
			else
			{
				// 1. QTE 조작 차단 및 기본 조작 복구
				if (IMC_QTE) Subsystem->RemoveMappingContext(IMC_QTE);
				if (IMC_Default) Subsystem->AddMappingContext(IMC_Default, 0);

				// 2. 물리 이동 복구
				if (GetCharacterMovement())
				{
					GetCharacterMovement()->SetMovementMode(MOVE_Walking);
				}
				UE_LOG(LogMyInput, Log, TEXT("[Input] 기본 입력 모드로 복구 완료"));
			}
		}
	}
}

void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC) return;

	if (IA_Move)
	{
		EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AMyCharacter::OnMove);
	}

	if (IA_Look)
	{
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AMyCharacter::OnLook);
	}

	if (IA_Jump)
	{
		EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &AMyCharacter::OnJump);
		EIC->BindAction(IA_Jump, ETriggerEvent::Completed, this, &AMyCharacter::OnJumpReleased);
		EIC->BindAction(IA_Jump, ETriggerEvent::Canceled, this, &AMyCharacter::OnJumpReleased);
	}

	if (IA_Interact)
	{     
		EIC->BindAction(IA_Interact, ETriggerEvent::Triggered, this, &AMyCharacter::OnInteractHoldProgress);
		EIC->BindAction(IA_Interact, ETriggerEvent::Completed, this, &AMyCharacter::OnInteractHoldCanceled);
		EIC->BindAction(IA_Interact, ETriggerEvent::Canceled, this, &AMyCharacter::OnInteractHoldCanceled);
       
		EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &AMyCharacter::OnQTEInteractStarted);
	}
    
	if (IA_QTE_Input)
	{
		EIC->BindAction(IA_QTE_Input, ETriggerEvent::Started, this, &AMyCharacter::OnQTEInput);
	}
}

void AMyCharacter::OnMove(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();

	const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	AddMovementInput(Forward, Axis.Y);
	AddMovementInput(Right, Axis.X);
}

void AMyCharacter::OnLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();

	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void AMyCharacter::OnJump(const FInputActionValue& Value)
{
	Jump();
}

void AMyCharacter::OnJumpReleased(const FInputActionValue& Value)
{
	StopJumping();
}

void AMyCharacter::InteractionCompleteResult()
{
	AActor* InteractedActor = InteractionComponent->GetLastInteractedActor();
	if (QuickSlotComponent && InteractedActor)
	{
		if (QuickSlotComponent->TryPickupItemToQuickSlot(InteractedActor))
		{
			InteractedActor->Destroy();
		}
	}
}

void AMyCharacter::OnInteractHoldProgress(const FInputActionValue& Value)
{
	if (InteractionComponent)
	{
		// 매 프레임 게이지를 채우고, 100%가 되었는지 bool 값으로 돌려받습니다.
		bool bIsComplete = InteractionComponent->HandleInteractHoldProgress(GetWorld()->GetDeltaSeconds());
       
		if (bIsComplete)
		{
			InteractionCompleteResult();
		}
	}
}

void AMyCharacter::OnInteractHoldCanceled(const FInputActionValue& Value)
{
	if (InteractionComponent)
	{
		InteractionComponent->HandleInteractHoldCanceled();
	}
}

void AMyCharacter::OnQTEInteractStarted(const FInputActionValue& Value)
{
	if (InteractionComponent)
	{
		InteractionComponent->HandleInteractQTEStarted();
	}
}

void AMyCharacter::OnQTEInput(const FInputActionValue& Value)
{
	if (!InteractionComponent) return;

	FVector2D InputVector = Value.Get<FVector2D>();
	EQTEDirection FinalDir = EQTEDirection::None;

	if (FMath::Abs(InputVector.X) > FMath::Abs(InputVector.Y))
	{
		FinalDir = (InputVector.X > 0) ? EQTEDirection::Right : EQTEDirection::Left;
	}
	else
	{
		FinalDir = (InputVector.Y > 0) ? EQTEDirection::Up : EQTEDirection::Down;
	}

	if (FinalDir != EQTEDirection::None)
	{
		InteractionComponent->ReceiveQTEInput(FinalDir);
		InteractionCompleteResult();
	}
}