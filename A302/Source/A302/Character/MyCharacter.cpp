// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/MyCharacter.h"

#include "Character/Components/InteractComponent.h"
#include "Character/Components/KnifeAutoTestComponent.h"
#include "Character/Components/QuickSlotComponent.h"
#include "EnhancedInputComponent.h"
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
		// 1. Triggered: 지정된 시간(Hold Time)을 끝까지 채웠을 때 1회 발생
		EIC->BindAction(IA_Interact, ETriggerEvent::Triggered, this, &AMyCharacter::OnInteractComplete);
       
		// 2. Ongoing: 키를 누르고 있는 동안 매 프레임 발생 (UI 게이지 업데이트용)
		EIC->BindAction(IA_Interact, ETriggerEvent::Ongoing, this, &AMyCharacter::OnInteractProgress);
       
		// 3. Canceled: 지정된 시간을 채우지 못하고 도중에 키를 뗐을 때 발생
		EIC->BindAction(IA_Interact, ETriggerEvent::Canceled, this, &AMyCharacter::OnInteractCanceled);
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

// void AMyCharacter::OnInteract(const FInputActionValue& Value)
// {
// 	FVector Start = GetPawnViewLocation();
// 	FVector ForwardVector = GetViewRotation().Vector();
// 	FVector End = Start + (ForwardVector * InteractionDistance);
// 	
// 	FHitResult HitResult;
// 	FCollisionQueryParams Params;
// 	Params.AddIgnoredActor(this);
// 	
// 	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
// 	{
// 		if (IInteractableInterface* Interactable = Cast<IInteractableInterface>(HitResult.GetActor()))
// 		{
// 			UE_LOG(LogTemp, Warning, TEXT("상호작용 키(F) 눌림! 대상: %s"), *HitResult.GetActor()->GetName());
// 			
// 			if (GEngine)
// 			{
// 				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("상호작용 로직 성공적으로 실행됨!"));
// 			}
// 		}
// 	}
// }

void AMyCharacter::OnInteractProgress(const FInputActionValue& Value)
{
	// Matches old MyCharacter::OnInteract flow:
	// 1) run interact, 2) try quick-slot pickup, 3) destroy actor on pickup success.
	if (!InteractionComponent)
	{
		return;
	}

	InteractionComponent->HandleInteractInput();

	AActor* InteractedActor = InteractionComponent->GetLastInteractedActor();
	if (QuickSlotComponent && InteractedActor)
	{
		if (QuickSlotComponent->TryPickupItemToQuickSlot(InteractedActor))
		{
			InteractedActor->Destroy();
		}
	}
}
