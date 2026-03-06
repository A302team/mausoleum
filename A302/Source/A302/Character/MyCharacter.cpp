// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/MyCharacter.h"
#include "Character/Components/CombatStatusComponent.h"
#include "Character/Components/InteractComponent.h"
#include "Character/Components/KnifeAutoTestComponent.h"
#include "Character/Components/MaliceComponent.h"
#include "Character/Components/QuickSlotComponent.h"
#include "Character/MyPlayerController.h"
#include "Engine/Engine.h"
#include "Voice/PrivateVoiceChatComponent.h"
#include "EnhancedInputComponent.h"
#include "GameData/ItemDefinition.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GamePlay/Items/ItemMalice.h"
#include "GamePlay/Items/ItemShield.h"
#include "GamePlay/Items/ItemTimeKnife.h"
#include "InputCoreTypes.h"
#include "InputAction.h"
#include "Object/BaseInteractable.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyInput, Log, All);

namespace
{
	void LogAndScreenCharacter(const FString& Message, const FColor& Color = FColor::Yellow, const float Duration = 3.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, Duration, Color, Message);
		}
	}
}

AMyCharacter::AMyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
    
	InteractionComponent = CreateDefaultSubobject<UInteractComponent>(TEXT("InteractionComponent"));
	QuickSlotComponent = CreateDefaultSubobject<UQuickSlotComponent>(TEXT("QuickSlotComponent"));
	CombatStatusComponent = CreateDefaultSubobject<UCombatStatusComponent>(TEXT("CombatStatusComponent"));
	MaliceComponent = CreateDefaultSubobject<UMaliceComponent>(TEXT("MaliceComponent"));
	KnifeAutoTestComponent = CreateDefaultSubobject<UKnifeAutoTestComponent>(TEXT("KnifeAutoTestComponent"));
	PrivateVoiceChatComponent = CreateDefaultSubobject<UPrivateVoiceChatComponent>(TEXT("VoiceChatComponent"));
}

void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (CombatStatusComponent)
	{
		CombatStatusComponent->OnShieldChanged.AddDynamic(this, &AMyCharacter::HandleShieldChanged);
		HandleShieldChanged(CombatStatusComponent->ShieldBlockCount);
	}

	if (MaliceComponent)
	{
		MaliceComponent->OnMaliceChanged.AddDynamic(this, &AMyCharacter::HandleMaliceChanged);
		HandleMaliceChanged(MaliceComponent->MaliceCount);
	}

	if (KnifeAutoTestComponent)
	{
		KnifeAutoTestComponent->StartAutoKnifeTest();
	}
}

void AMyCharacter::HandleShieldChanged(int32 NewCount)
{
	if (AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(GetController()))
	{
		MyPlayerController->UpdateShieldCountText(FMath::Max(0, NewCount));
	}
}

void AMyCharacter::HandleMaliceChanged(int32 NewCount)
{
	bMaliceEnding = NewCount >= 3;
	bNiceEnding = !bMaliceEnding;

	if (AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(GetController()))
	{
		MyPlayerController->UpdateMaliceCountText(FMath::Max(0, NewCount));
	}
}

float AMyCharacter::TakeDamage(
	float DamageAmount,
	const FDamageEvent& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser
)
{
	if (bIsDead)
	{
		return 0.f;
	}

	if (QuickSlotComponent && QuickSlotComponent->TryAutoUseItem())
	{
		LogAndScreenCharacter(TEXT("[MyCharacter] Blocked by auto-used shield"), FColor::Green, 1.5f);
		return 0.f;
	}

	HandleDead();

	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void AMyCharacter::NotifyKilledCharacter()
{
	CompleteTimedKnifeObjective();
}

void AMyCharacter::CompleteTimedKnifeObjective()
{
	if (!bHasActiveTimedKnife)
	{
		return;
	}

	if (QuickSlotComponent && !ActiveTimedKnifeItemId.IsNone())
	{
		QuickSlotComponent->RemoveFirstItemByItemId(ActiveTimedKnifeItemId);
	}

	LogAndScreenCharacter(TEXT("[MyCharacter] Timed knife success"), FColor::Green, 2.0f);
	ClearTimedKnifeState(true);
}

void AMyCharacter::StartTimedKnifeCountdown(const UItemDefinition* TimedKnifeDefinition)
{
	if (!TimedKnifeDefinition)
	{
		return;
	}

	const float Duration = FMath::Max(1.0f, TimedKnifeDefinition->TimedKillDuration);
	bHasActiveTimedKnife = true;
	TimedKnifeRemainingSeconds = Duration;
	ActiveTimedKnifeItemId = TimedKnifeDefinition->ItemId;

	if (AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(GetController()))
	{
		MyPlayerController->UpdateItemTimerText(TimedKnifeRemainingSeconds);
		MyPlayerController->SetItemTimerVisible(true);
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimedKnifeTimerHandle);
		World->GetTimerManager().SetTimer(
			TimedKnifeTimerHandle,
			this,
			&AMyCharacter::TickTimedKnifeCountdown,
			1.0f,
			true
		);
	}
}

void AMyCharacter::TickTimedKnifeCountdown()
{
	if (!bHasActiveTimedKnife)
	{
		return;
	}

	TimedKnifeRemainingSeconds = FMath::Max(0.0f, TimedKnifeRemainingSeconds - 1.0f);

	if (AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(GetController()))
	{
		MyPlayerController->UpdateItemTimerText(TimedKnifeRemainingSeconds);
	}

	if (TimedKnifeRemainingSeconds > 0.0f)
	{
		return;
	}

	if (QuickSlotComponent && !ActiveTimedKnifeItemId.IsNone())
	{
		QuickSlotComponent->RemoveFirstItemByItemId(ActiveTimedKnifeItemId);
	}

	ClearTimedKnifeState(true);
	HandleDead();
}

void AMyCharacter::ClearTimedKnifeState(bool bHideTimer)
{
	bHasActiveTimedKnife = false;
	TimedKnifeRemainingSeconds = 0.0f;
	ActiveTimedKnifeItemId = NAME_None;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimedKnifeTimerHandle);
	}

	if (bHideTimer)
	{
		if (AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(GetController()))
		{
			MyPlayerController->SetItemTimerVisible(false);
		}
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

void AMyCharacter::HandleDead()
{
	if (bIsDead)
	{
		return;
	}

	ClearTimedKnifeState(true);
	bIsDead = true;
	LogAndScreenCharacter(TEXT("[MyCharacter] Dead"), FColor::Red, 4.0f);
}

void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

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
		EIC->BindAction(IA_Interact, ETriggerEvent::Triggered, this, &AMyCharacter::OnInteractHoldProgress);
		EIC->BindAction(IA_Interact, ETriggerEvent::Completed, this, &AMyCharacter::OnInteractHoldCanceled);
		EIC->BindAction(IA_Interact, ETriggerEvent::Canceled, this, &AMyCharacter::OnInteractHoldCanceled);
        
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

	if (CombatStatusComponent)
	{
		HandleShieldChanged(CombatStatusComponent->ShieldBlockCount);
	}

	if (MaliceComponent)
	{
		HandleMaliceChanged(MaliceComponent->MaliceCount);
	}

	if (AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(GetController()))
	{
		if (bHasActiveTimedKnife)
		{
			MyPlayerController->UpdateItemTimerText(TimedKnifeRemainingSeconds);
			MyPlayerController->SetItemTimerVisible(true);
		}
		else
		{
			MyPlayerController->SetItemTimerVisible(false);
		}
	}

	if (IA_VoiceChat)
	{
		EIC->BindAction(IA_VoiceChat, ETriggerEvent::Started, this, &AMyCharacter::OnToggleVoiceChat);
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
	UE_LOG(LogTemp, Warning, TEXT("Interaction Result Started! Actor: %s"), InteractedActor ? *InteractedActor->GetName() : TEXT("None"));
	if (!InteractedActor)
	{
		return;
	}

	const ABaseInteractable* Interactable = Cast<ABaseInteractable>(InteractedActor);
	const UItemDefinition* PickedItemDefinition = Interactable ? Interactable->GetItemDefinition() : nullptr;
	const bool bIsMalicePickup =
		PickedItemDefinition &&
		(
			PickedItemDefinition->bApplyMaliceOnPickup ||
			(
				PickedItemDefinition->ItemLogicClass &&
				PickedItemDefinition->ItemLogicClass->IsChildOf(UItemMalice::StaticClass())
			)
		);

	if (bIsMalicePickup)
	{
		if (MaliceComponent)
		{
			const int32 MaliceAmount = PickedItemDefinition->bApplyMaliceOnPickup
				? PickedItemDefinition->MaliceAmount
				: 1;
			MaliceComponent->AddMalice(FMath::Max(1, MaliceAmount));
		}

		InteractedActor->Destroy();
		return;
	}

	if (QuickSlotComponent && QuickSlotComponent->TryPickupItemToQuickSlot(InteractedActor))
	{
		InteractedActor->Destroy();

		const bool bIsShieldItem =
			PickedItemDefinition &&
			PickedItemDefinition->ItemLogicClass &&
			PickedItemDefinition->ItemLogicClass->IsChildOf(UItemShield::StaticClass());

		if (bIsShieldItem && CombatStatusComponent)
		{
			CombatStatusComponent->AddShield(FMath::Max(1, PickedItemDefinition->BlockCount));
		}

		const bool bIsTimedKillKnife =
			PickedItemDefinition &&
			(
				PickedItemDefinition->bIsTimedKillKnife ||
				(
					PickedItemDefinition->ItemLogicClass &&
					PickedItemDefinition->ItemLogicClass->IsChildOf(UItemTimeKnife::StaticClass())
				)
			);

		if (bIsTimedKillKnife)
		{
			StartTimedKnifeCountdown(PickedItemDefinition);
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
		const bool bUsedTimedKillKnife =
			UsedItemDefinition &&
			(
				UsedItemDefinition->bIsTimedKillKnife ||
				(
					UsedItemDefinition->ItemLogicClass &&
					UsedItemDefinition->ItemLogicClass->IsChildOf(UItemTimeKnife::StaticClass())
				)
			);

		if (bUsedTimedKillKnife)
		{
			CompleteTimedKnifeObjective();
		}

		BP_OnPrimaryItemUsed(UsedItemDefinition, UsedSlotIndex + 1);
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
		if (InteractionComponent->ReceiveQTEInput(FinalDir))
		{
			InteractionCompleteResult();
		}
	}
}
