// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/MyCharacter.h"

#include "Character/Components/KnifeAutoTestComponent.h"
#include "Character/Components/InteractionQuickSlotComponent.h"
#include "Character/Components/PrivateVoiceChatComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Controller.h"
#include "InputAction.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyInput, Log, All);

AMyCharacter::AMyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionQuickSlotComponent = CreateDefaultSubobject<UInteractionQuickSlotComponent>(
		TEXT("InteractionQuickSlotComponent")
	);
	KnifeAutoTestComponent = CreateDefaultSubobject<UKnifeAutoTestComponent>(
		TEXT("KnifeAutoTestComponent")
	);
	PrivateVoiceChatComponent = CreateDefaultSubobject<UPrivateVoiceChatComponent>(
		TEXT("PrivateVoiceChatComponent")
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

void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UE_LOG(LogMyInput, Warning, TEXT("[Input] SetupPlayerInputComponent called. PC=%s"), *GetNameSafe(PlayerInputComponent));

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	UE_LOG(LogMyInput, Warning, TEXT("[Input] EnhancedInputComponent cast: %s"), EIC ? TEXT("OK") : TEXT("FAILED"));

	UE_LOG(LogMyInput, Warning, TEXT("[Input] IA_Move=%s IA_Look=%s IA_Jump=%s"), *GetNameSafe(IA_Move), *GetNameSafe(IA_Look), *GetNameSafe(IA_Jump));

	if (!EIC)
	{
		return;
	}

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
		EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &AMyCharacter::OnInteract);
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

void AMyCharacter::OnInteract(const FInputActionValue& Value)
{
	if (InteractionQuickSlotComponent)
	{
		InteractionQuickSlotComponent->HandleInteractInput();
	}
}
