// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/MyCharacter.h"

#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "GameFramework/Controller.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyInput, Log, All);

// Sets default values
AMyCharacter::AMyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UE_LOG(LogMyInput, Warning, TEXT("[Input] SetupPlayerInputComponent called. PC=%s"),
        *GetNameSafe(PlayerInputComponent));

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    UE_LOG(LogMyInput, Warning, TEXT("[Input] EnhancedInputComponent cast: %s"),
        EIC ? TEXT("OK") : TEXT("FAILED"));

    UE_LOG(LogMyInput, Warning, TEXT("[Input] IA_Move=%s IA_Look=%s IA_Jump=%s"),
        *GetNameSafe(IA_Move), *GetNameSafe(IA_Look), *GetNameSafe(IA_Jump));

    if (!EIC) return;

    if (IA_Move)
        EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AMyCharacter::OnMove);

    if (IA_Look)
        EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AMyCharacter::OnLook);

    if (IA_Jump)
    {
        EIC->BindAction(IA_Jump, ETriggerEvent::Started,   this, &AMyCharacter::OnJump);
        EIC->BindAction(IA_Jump, ETriggerEvent::Completed, this, &AMyCharacter::OnJumpReleased);
        EIC->BindAction(IA_Jump, ETriggerEvent::Canceled,  this, &AMyCharacter::OnJumpReleased);
    }
}

void AMyCharacter::OnMove(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>(); // IA_Move: Axis2D

	// 컨트롤러 yaw 기준으로 전/우 벡터 계산
	const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	// X=좌우, Y=앞뒤
	AddMovementInput(Forward, Axis.Y);
	AddMovementInput(Right, Axis.X);

}

void AMyCharacter::OnLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>(); // IA_Look: Axis2D

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