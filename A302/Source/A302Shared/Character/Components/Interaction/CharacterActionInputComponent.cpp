#include "Character/Components/Interaction/CharacterActionInputComponent.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "Character/Components/Combat/CharacterHealthComponent.h"
#include "Character/Components/Inventory/QuickSlotComponent.h"
#include "Character/Components/Inventory/ItemTargetingComponent.h"
#include "Character/Components/Interaction/CharacterRewardComponent.h"
#include "Character/Components/Interaction/InteractComponent.h"
#include "Character/Components/Combat/EquipmentComponent.h"
#include "Animation/AnimMontage.h"
#include "Animation/MyAnimInstance.h"
#include "Interface/A302AnimationBridge.h"
#include "GamePlay/Items/BaseItem.h"
#include "GamePlay/Items/ItemKnife.h"
#include "GamePlay/Items/ItemCursedSword.h"
#include "GameData/Items/ItemDefinition.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

UCharacterActionInputComponent::UCharacterActionInputComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCharacterActionInputComponent::BeginPlay()
{
	Super::BeginPlay();
}

AMyCharacter* UCharacterActionInputComponent::GetOwnerCharacter() const
{
	return Cast<AMyCharacter>(GetOwner());
}

void UCharacterActionInputComponent::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter) return;

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC) return;

	// 이동 및 시점
	if (OwnerCharacter->IA_Move) EIC->BindAction(OwnerCharacter->IA_Move, ETriggerEvent::Triggered, this, &UCharacterActionInputComponent::OnMove);
	if (OwnerCharacter->IA_Look) EIC->BindAction(OwnerCharacter->IA_Look, ETriggerEvent::Triggered, this, &UCharacterActionInputComponent::OnLook);
	
	// 점프
	if (OwnerCharacter->IA_Jump)
	{
		EIC->BindAction(OwnerCharacter->IA_Jump, ETriggerEvent::Started, this, &UCharacterActionInputComponent::OnJump);
		EIC->BindAction(OwnerCharacter->IA_Jump, ETriggerEvent::Completed, this, &UCharacterActionInputComponent::OnJumpReleased);
		EIC->BindAction(OwnerCharacter->IA_Jump, ETriggerEvent::Canceled, this, &UCharacterActionInputComponent::OnJumpReleased);
	}

	// 상호작용 (InteractComponent로 바로 라우팅)
	UInteractComponent* InteractComp = OwnerCharacter->FindComponentByClass<UInteractComponent>();
	if (OwnerCharacter->IA_Interact && InteractComp)
	{
		EIC->BindAction(OwnerCharacter->IA_Interact, ETriggerEvent::Triggered, InteractComp, &UInteractComponent::OnInteractHoldProgress);
		EIC->BindAction(OwnerCharacter->IA_Interact, ETriggerEvent::Completed, InteractComp, &UInteractComponent::OnInteractHoldCanceled);
		EIC->BindAction(OwnerCharacter->IA_Interact, ETriggerEvent::Canceled, InteractComp, &UInteractComponent::OnInteractHoldCanceled);
		EIC->BindAction(OwnerCharacter->IA_Interact, ETriggerEvent::Started, InteractComp, &UInteractComponent::OnQTEInteractStarted);
	}

	// QTE
	if (OwnerCharacter->IA_QTE_Input) EIC->BindAction(OwnerCharacter->IA_QTE_Input, ETriggerEvent::Started, this, &UCharacterActionInputComponent::OnQTEInput);

	// 아이템 및 공격
	if (OwnerCharacter->IA_ItemSelect)
	{
		EIC->BindAction(OwnerCharacter->IA_ItemSelect, ETriggerEvent::Started, this, &UCharacterActionInputComponent::OnItemSelect);
		EIC->BindAction(OwnerCharacter->IA_ItemSelect, ETriggerEvent::Triggered, this, &UCharacterActionInputComponent::OnItemSelect);
	}
	if (OwnerCharacter->IA_Attack)
	{
		EIC->BindAction(OwnerCharacter->IA_Attack, ETriggerEvent::Started, this, &UCharacterActionInputComponent::OnAttack);
		EIC->BindAction(OwnerCharacter->IA_Attack, ETriggerEvent::Started, this, &UCharacterActionInputComponent::OnUseSelectedItem);
	}

	// UI 및 보이스챗
	if (OwnerCharacter->IA_VoiceChat) EIC->BindAction(OwnerCharacter->IA_VoiceChat, ETriggerEvent::Started, this, &UCharacterActionInputComponent::OnToggleVoiceChat);
	if (OwnerCharacter->IA_ESC)       EIC->BindAction(OwnerCharacter->IA_ESC, ETriggerEvent::Started, this, &UCharacterActionInputComponent::OnEscPressed);
}

void UCharacterActionInputComponent::OnMove(const FInputActionValue& Value)
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter) return;

	if (OwnerCharacter->IsDead()) return;

	if (const APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
	{
		if (PC->IsMoveInputIgnored()) return;
	}

	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator YawRot(0.f, OwnerCharacter->GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	OwnerCharacter->AddMovementInput(Forward, Axis.Y);
	OwnerCharacter->AddMovementInput(Right, Axis.X);
}

void UCharacterActionInputComponent::OnLook(const FInputActionValue& Value)
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter) return;
	if (OwnerCharacter->IsDead()) return;

	const FVector2D Axis = Value.Get<FVector2D>();
	float LookSensitivityMultiplier = 1.0f;
	
	if (const AMyPlayerController* MyPC = Cast<AMyPlayerController>(OwnerCharacter->GetController()))
	{
		LookSensitivityMultiplier = MyPC->GetMouseSensitivityMultiplier();
	}

	OwnerCharacter->AddControllerYawInput(Axis.X * LookSensitivityMultiplier);
	OwnerCharacter->AddControllerPitchInput(Axis.Y * LookSensitivityMultiplier);
}

void UCharacterActionInputComponent::OnJump(const FInputActionValue& Value)
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter) return;

	if (OwnerCharacter->IsDead()) return;

	if (const APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
	{
		if (PC->IsMoveInputIgnored()) return;
	}

	OwnerCharacter->Jump();
}

void UCharacterActionInputComponent::OnJumpReleased(const FInputActionValue& Value)
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter) return;

	if (OwnerCharacter->IsDead()) return;

	OwnerCharacter->StopJumping();
}

void UCharacterActionInputComponent::OnItemSelect(const FInputActionValue& Value)
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter) return;

	UQuickSlotComponent* QuickSlotComp = OwnerCharacter->FindComponentByClass<UQuickSlotComponent>();
	if (!QuickSlotComp) return;

	// Enhanced Input Modifiers(Scalar: 1.0~6.0)를 통해 전달된 슬롯 번호를 직접 사용합니다.
	// 개발자 노트: IMC(Input Mapping Context)에서 Key 1~6 각각에 대해 IA_ItemSelect의 Modifier -> Scalar 값을 1.0~6.0으로 설정해야 합니다.
	const int32 SlotNumberOneBased = FMath::RoundToInt(Value.Get<float>());

	if (SlotNumberOneBased >= 1 && SlotNumberOneBased <= 6)
	{
		if (QuickSlotComp->GetSelectedSlotIndex() != SlotNumberOneBased - 1)
		{
			QuickSlotComp->SelectQuickSlotByNumber(SlotNumberOneBased);
		}
	}
}

void UCharacterActionInputComponent::OnAttack(const FInputActionValue& Value)
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter) return;

	if (OwnerCharacter->IsDead())
	{
		if (AMyPlayerController* OwnerController = Cast<AMyPlayerController>(OwnerCharacter->GetController()))
		{
			OwnerController->CycleAlivePlayerViewTarget();
		}
		return;
	}

	UQuickSlotComponent* QuickSlotComp = nullptr;
	UItemDefinition* SelectedItemDefinition = nullptr;
	UClass* SelectedLogicClass = nullptr;
	if (!TryGetSelectedItemContext(QuickSlotComp, SelectedItemDefinition, SelectedLogicClass))
	{
		return;
	}

	if (!IsAttackItemLogicClass(SelectedLogicClass))
	{
		return;
	}

	UEquipmentComponent* EquipmentComp = OwnerCharacter->FindComponentByClass<UEquipmentComponent>();

	UItemDefinition* UsedItemDefinition = nullptr;
	int32 UsedSlotIndex = INDEX_NONE;
	
	if (QuickSlotComp->TryUseSelectedItem(UsedItemDefinition, UsedSlotIndex))
	{
		TryHandleTargetedServerUse(OwnerCharacter, UsedItemDefinition, true);
		HandlePrimaryItemUseFeedback(OwnerCharacter, UsedItemDefinition, UsedSlotIndex);

		OwnerCharacter->SetCameraViewMode(EA302CameraViewMode::ThirdPerson);

		UClass* UsedLogicClass = UsedItemDefinition ? UsedItemDefinition->ResolveRewardLogicClass() : nullptr;
		const bool bIsCursedSword = UsedLogicClass && UsedLogicClass->IsChildOf(UItemCursedSword::StaticClass());
		
		if (IA302AnimationBridge* Anim = Cast<IA302AnimationBridge>(OwnerCharacter->GetMesh()->GetAnimInstance()))
		{
			if (EquipmentComp)
			{
				bIsCursedSword ? EquipmentComp->EquipCursedSwordWeapon() : EquipmentComp->EquipKnifeWeapon();
			}

			bIsCursedSword ? Anim->PlayTimeKnifeAnimation() : Anim->PlayAttackAnimation();

			if (UMyAnimInstance* MyAnimInstance = Cast<UMyAnimInstance>(OwnerCharacter->GetMesh()->GetAnimInstance()))
			{
				UAnimMontage* AttackMontage = bIsCursedSword ? MyAnimInstance->TimeKnifeMontage : MyAnimInstance->AttackMontage;
				if (AttackMontage)
				{
					FOnMontageEnded MontageEndedDelegate;
					MontageEndedDelegate.BindUObject(this, &UCharacterActionInputComponent::HandleAttackMontageEnded);
					MyAnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, AttackMontage);

					BeginAttackInputLock();
				}
			}
		}

		ScheduleAttackCameraRecovery(OwnerCharacter, bIsCursedSword);
	}
}

void UCharacterActionInputComponent::OnUseSelectedItem(const FInputActionValue& Value)
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || OwnerCharacter->IsDead())
	{
		return;
	}

	UQuickSlotComponent* QuickSlotComp = nullptr;
	UItemDefinition* SelectedItemDefinition = nullptr;
	UClass* SelectedLogicClass = nullptr;
	if (!TryGetSelectedItemContext(QuickSlotComp, SelectedItemDefinition, SelectedLogicClass))
	{
		return;
	}

	if (IsAttackItemLogicClass(SelectedLogicClass))
	{
		return;
	}

	UItemDefinition* UsedItemDefinition = nullptr;
	int32 UsedSlotIndex = INDEX_NONE;
	if (!QuickSlotComp->TryUseSelectedItem(UsedItemDefinition, UsedSlotIndex))
	{
		return;
	}

	TryHandleTargetedServerUse(OwnerCharacter, UsedItemDefinition, false);
	HandlePrimaryItemUseFeedback(OwnerCharacter, UsedItemDefinition, UsedSlotIndex);
}

void UCharacterActionInputComponent::OnToggleVoiceChat(const FInputActionValue& Value)
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter) return;

	if (AMyPlayerController* ClientEventBridge = Cast<AMyPlayerController>(OwnerCharacter->GetController()))
	{
		ClientEventBridge->ToggleVoiceChatCapture();
	}
}

void UCharacterActionInputComponent::OnEscPressed(const FInputActionValue& Value)
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter) return;

	if (AMyPlayerController* ClientEventBridge = Cast<AMyPlayerController>(OwnerCharacter->GetController()))
	{
		ClientEventBridge->ToggleInGameSettingMenu();
	}
}

void UCharacterActionInputComponent::OnQTEInput(const FInputActionValue& Value)
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter) return;

	if (UInteractComponent* InteractionComponent = OwnerCharacter->FindComponentByClass<UInteractComponent>())
	{
		InteractionComponent->OnQTEInput(Value.Get<FVector2D>());
	}
}

bool UCharacterActionInputComponent::TryGetSelectedItemContext(
	UQuickSlotComponent*& OutQuickSlotComp,
	UItemDefinition*& OutItemDefinition,
	UClass*& OutLogicClass
) const
{
	OutQuickSlotComp = nullptr;
	OutItemDefinition = nullptr;
	OutLogicClass = nullptr;

	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		return false;
	}

	OutQuickSlotComp = OwnerCharacter->FindComponentByClass<UQuickSlotComponent>();
	if (!OutQuickSlotComp)
	{
		return false;
	}

	const int32 SelectedSlotIndex = OutQuickSlotComp->GetSelectedSlotIndex();
	if (SelectedSlotIndex == INDEX_NONE)
	{
		return false;
	}

	OutItemDefinition = const_cast<UItemDefinition*>(OutQuickSlotComp->GetItemDefinitionAtSlot(SelectedSlotIndex));
	if (!OutItemDefinition)
	{
		return false;
	}

	OutLogicClass = OutItemDefinition->ResolveRewardLogicClass();
	return OutLogicClass != nullptr;
}

bool UCharacterActionInputComponent::IsAttackItemLogicClass(const UClass* LogicClass) const
{
	return LogicClass && LogicClass->IsChildOf(UItemKnife::StaticClass());
}

void UCharacterActionInputComponent::HandlePrimaryItemUseFeedback(
	AMyCharacter* OwnerCharacter,
	UItemDefinition* UsedItemDefinition,
	int32 UsedSlotIndex
) const
{
	if (!OwnerCharacter || !UsedItemDefinition || UsedSlotIndex == INDEX_NONE)
	{
		return;
	}

	OwnerCharacter->BP_OnPrimaryItemUsed(UsedItemDefinition, UsedSlotIndex + 1);
}

void UCharacterActionInputComponent::TryHandleTargetedServerUse(
	AMyCharacter* OwnerCharacter,
	const UItemDefinition* UsedItemDefinition,
	bool bIsAttackItem
) const
{
	if (!OwnerCharacter || !UsedItemDefinition)
	{
		return;
	}

	const bool bNeedsServerTargetUse =
		UsedItemDefinition->Payload.UseMode == EItemUseMode::Targeted ||
		UsedItemDefinition->Payload.UseMode == EItemUseMode::SelfOrTargeted;
	if (!bNeedsServerTargetUse)
	{
		return;
	}

	UItemTargetingComponent* TargetingComponent = OwnerCharacter->FindComponentByClass<UItemTargetingComponent>();
	if (!TargetingComponent)
	{
		return;
	}

	FItemTargetData TargetData;
	const bool bForceTargetActor = UsedItemDefinition->Payload.UseMode == EItemUseMode::Targeted;
	if (!TargetingComponent->TryBuildTargetDataForUse(UsedItemDefinition, TargetData, bForceTargetActor))
	{
		return;
	}

	if (!OwnerCharacter->HasAuthority())
	{
		if (UCharacterRewardComponent* RewardComponent = OwnerCharacter->FindComponentByClass<UCharacterRewardComponent>())
		{
			RewardComponent->Server_RequestTargetedItemUse(const_cast<UItemDefinition*>(UsedItemDefinition), TargetData.TargetActor);
		}
		return;
	}

	if (bIsAttackItem)
	{
		return;
	}

	UClass* LogicClass = UsedItemDefinition->ResolveRewardLogicClass();
	const UBaseItem* ItemLogic = LogicClass ? Cast<UBaseItem>(LogicClass->GetDefaultObject()) : nullptr;
	if (!ItemLogic)
	{
		return;
	}

	FString SystemMessage;
	if (!ItemLogic->ResolveServerTargetedUse(OwnerCharacter, TargetData.TargetActor, SystemMessage))
	{
		return;
	}

	if (!SystemMessage.IsEmpty())
	{
		if (AMyPlayerController* OwnerPlayerController = Cast<AMyPlayerController>(OwnerCharacter->GetController()))
		{
			OwnerPlayerController->Client_ReceiveSystemMessage(SystemMessage);
		}
	}
}

void UCharacterActionInputComponent::ScheduleAttackCameraRecovery(AMyCharacter* OwnerCharacter, bool bIsCursedSword)
{
	if (!OwnerCharacter)
	{
		return;
	}

	float RecoveryDelay = DefaultAttackCameraRecoveryDelay;

	if (const USkeletalMeshComponent* MeshComponent = OwnerCharacter->GetMesh())
	{
		if (const UMyAnimInstance* MyAnimInstance = Cast<UMyAnimInstance>(MeshComponent->GetAnimInstance()))
		{
			const UAnimMontage* AttackMontage = bIsCursedSword ? MyAnimInstance->TimeKnifeMontage : MyAnimInstance->AttackMontage;
			if (AttackMontage)
			{
				RecoveryDelay = FMath::Max(DefaultAttackCameraRecoveryDelay, AttackMontage->GetPlayLength());
			}
		}
	}

	if (UWorld* World = GetWorld())
	{
		PendingCameraRecoveryOwner = OwnerCharacter;
		World->GetTimerManager().ClearTimer(AttackCameraRecoveryTimerHandle);
		World->GetTimerManager().SetTimer(
			AttackCameraRecoveryTimerHandle,
			this,
			&UCharacterActionInputComponent::HandleAttackCameraRecovery,
			RecoveryDelay,
			false
		);
	}
}

void UCharacterActionInputComponent::HandleAttackCameraRecovery()
{
	if (PendingCameraRecoveryOwner.IsValid())
	{
		PendingCameraRecoveryOwner->SetCameraViewMode(EA302CameraViewMode::FirstPersonChest);
	}

	PendingCameraRecoveryOwner.Reset();
}

void UCharacterActionInputComponent::BeginAttackInputLock()
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PC)
	{
		return;
	}

	bAttackInputLocked = true;

	if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		if (OwnerCharacter->IMC_Default)
		{
			Subsystem->RemoveMappingContext(OwnerCharacter->IMC_Default);
		}

		if (OwnerCharacter->IMC_AttackLock)
		{
			Subsystem->AddMappingContext(OwnerCharacter->IMC_AttackLock, 1);
		}
	}
}

void UCharacterActionInputComponent::EndAttackInputLock()
{
	if (!bAttackInputLocked)
	{
		return;
	}

	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		bAttackInputLocked = false;
		return;
	}

	APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PC)
	{
		bAttackInputLocked = false;
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		if (OwnerCharacter->IMC_AttackLock)
		{
			Subsystem->RemoveMappingContext(OwnerCharacter->IMC_AttackLock);
		}

		if (OwnerCharacter->IMC_Default)
		{
			Subsystem->AddMappingContext(OwnerCharacter->IMC_Default, 0);
		}
	}

	bAttackInputLocked = false;
}

void UCharacterActionInputComponent::HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	EndAttackInputLock();
}
