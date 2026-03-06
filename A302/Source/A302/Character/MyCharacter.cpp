// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/MyCharacter.h"

#include "Character/Components/InteractComponent.h"
#include "Character/Components/KnifeAutoTestComponent.h"
#include "Character/Components/QuickSlotComponent.h"
#include "Voice/PrivateVoiceChatComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
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
	PrivateVoiceChatComponent = CreateDefaultSubobject<UPrivateVoiceChatComponent>(
		TEXT("VoiceChatComponent")
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

void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UE_LOG(LogMyInput, Warning, TEXT("[Input] SetupPlayerInputComponent called. PC=%s"), *GetNameSafe(PlayerInputComponent));

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	UE_LOG(LogMyInput, Warning, TEXT("[Input] EnhancedInputComponent cast: %s"), EIC ? TEXT("OK") : TEXT("FAILED"));

	UE_LOG(
		LogMyInput,
		Warning,
		TEXT("[Input] IA_Move=%s IA_Look=%s IA_Jump=%s IA_Interact=%s IA_ItemSelect=%s IA_Attack=%s"),
		*GetNameSafe(IA_Move),
		*GetNameSafe(IA_Look),
		*GetNameSafe(IA_Jump),
		*GetNameSafe(IA_Interact),
		*GetNameSafe(IA_ItemSelect),
		*GetNameSafe(IA_Attack)
	);

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
		// Hold
		EIC->BindAction(IA_Interact, ETriggerEvent::Triggered, this, &AMyCharacter::OnInteractHoldComplete);
		EIC->BindAction(IA_Interact, ETriggerEvent::Ongoing, this, &AMyCharacter::OnInteractHoldProgress);
		EIC->BindAction(IA_Interact, ETriggerEvent::Canceled, this, &AMyCharacter::OnInteractHoldCanceled);
		
		// QTE
		EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &AMyCharacter::OnQTEInteractStarted);
	}

	if (IA_ItemSelect)
	{
		EIC->BindAction(IA_ItemSelect, ETriggerEvent::Started, this, &AMyCharacter::OnItemSelect);
		EIC->BindAction(IA_ItemSelect, ETriggerEvent::Triggered, this, &AMyCharacter::OnItemSelect);
	}

	if (IA_Attack)
	{
		EIC->BindAction(IA_Attack, ETriggerEvent::Started, this, &AMyCharacter::OnAttack);
	}

	if(IA_VoiceChat){
		EIC->BindAction(IA_VoiceChat, ETriggerEvent::Started, this, &AMyCharacter::OnToggleVoiceChat);
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

void AMyCharacter::OnInteractHoldProgress(const FInputActionValue& Value)
{
	if (InteractionComponent)
	{
		InteractionComponent->HandleInteractHoldProgress(GetWorld()->GetDeltaSeconds());
	}
}

void AMyCharacter::OnInteractHoldComplete(const FInputActionValue& Value)
{
	if (!InteractionComponent) return;

	InteractionComponent->HandleInteractHoldComplete();

	AActor* InteractedActor = InteractionComponent->GetLastInteractedActor();
	if (QuickSlotComponent && InteractedActor)
	{
		if (QuickSlotComponent->TryPickupItemToQuickSlot(InteractedActor))
		{
			InteractedActor->Destroy();
		}
	}
}

void AMyCharacter::OnItemSelect(const FInputActionValue& Value)
{
	if (!QuickSlotComponent)
	{
		return;
	}

	int32 SlotNumberOneBased = INDEX_NONE;

	// Fallback path: read actual key states so slot selection still works even when
	// IA_ItemSelect value scaling in IMC is not configured as 1~5.
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->IsInputKeyDown(EKeys::One) || PC->IsInputKeyDown(EKeys::NumPadOne))
		{
			SlotNumberOneBased = 1;
		}
		else if (PC->IsInputKeyDown(EKeys::Two) || PC->IsInputKeyDown(EKeys::NumPadTwo))
		{
			SlotNumberOneBased = 2;
		}
		else if (PC->IsInputKeyDown(EKeys::Three) || PC->IsInputKeyDown(EKeys::NumPadThree))
		{
			SlotNumberOneBased = 3;
		}
		else if (PC->IsInputKeyDown(EKeys::Four) || PC->IsInputKeyDown(EKeys::NumPadFour))
		{
			SlotNumberOneBased = 4;
		}
		else if (PC->IsInputKeyDown(EKeys::Five) || PC->IsInputKeyDown(EKeys::NumPadFive))
		{
			SlotNumberOneBased = 5;
		}
	}

	if (SlotNumberOneBased == INDEX_NONE)
	{
		const float AxisValue = Value.Get<float>();
		if (!FMath::IsNearlyZero(AxisValue))
		{
			SlotNumberOneBased = FMath::RoundToInt(AxisValue);
		}
	}

	if (SlotNumberOneBased == INDEX_NONE)
	{
		return;
	}

	if (QuickSlotComponent->GetSelectedSlotIndex() == SlotNumberOneBased - 1)
	{
		return;
	}

	QuickSlotComponent->SelectQuickSlotByNumber(SlotNumberOneBased);
}

void AMyCharacter::OnAttack(const FInputActionValue& Value)
{
	if (!QuickSlotComponent)
	{
		return;
	}

	UItemDefinition* UsedItemDefinition = nullptr;
	int32 UsedSlotIndex = INDEX_NONE;
	if (QuickSlotComponent->TryUseSelectedItem(UsedItemDefinition, UsedSlotIndex))
	{
		BP_OnPrimaryItemUsed(UsedItemDefinition, UsedSlotIndex + 1);
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
/**
 * @brief V키를 입력 받아, 마이크 껐다 켰다하기
 * 
 * @param Value 
 */
void AMyCharacter::OnToggleVoiceChat(const FInputActionValue& Value)
{
	PrivateVoiceChatComponent = FindComponentByClass<UPrivateVoiceChatComponent>();
	if (PrivateVoiceChatComponent)
	{
		PrivateVoiceChatComponent->ToggleMicrophone();
	}
}