#include "Character/MyCharacter.h"

#include "Animation/MyAnimInstance.h"
#include "Character/Components/CombatStatusComponent.h"
#include "Character/Components/InteractComponent.h"
#include "Character/Components/ItemManagerComponent.h"
#include "Character/Components/ItemTargetingComponent.h"
#include "Character/Components/KnifeAutoTestComponent.h"
#include "Character/Components/MaliceComponent.h"
#include "Character/Components/QuickSlotComponent.h"
#include "Character/MyPlayerController.h"
#include "Engine/Engine.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameData/Items/ItemDefinition.h"
#include "GameData/Events/PersonalEvents/PersonalEventInspectMaliceDefinition.h"
#include "GameData/Events/PersonalEvents/PersonalEventMaliceDefinition.h"
#include "GameData/Events/PersonalEvents/PersonalEventPublicMaliceDefinition.h"
#include "GameData/Events/PersonalEvents/PersonalEventTimeKnifeDefinition.h"
#include "GameData/RewardDefinition.h"
#include "GameData/RewardTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GamePlay/Actor/KnifeActor.h"
#include "GamePlay/Events/GroupEvents/BaseGroupEvent.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"
#include "GamePlay/Events/PersonalEvents/PersonalEventInspectMalice.h"
#include "GamePlay/Events/PersonalEvents/PersonalEventMalice.h"
#include "GamePlay/Events/PersonalEvents/PersonalEventPublicMalice.h"
#include "GamePlay/Events/PersonalEvents/PersonalEventTimeKnife.h"
#include "GamePlay/Items/ItemShield.h"
#include "GamePlay/Items/ItemTimeKnife.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "Net/UnrealNetwork.h"
#include "Object/BaseInteractable.h"
#include "Voice/PrivateVoiceChatComponent.h"

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
	bReplicates = true;

	SetReplicateMovement(true);

	ItemManagerComponent = CreateDefaultSubobject<UItemManagerComponent>(TEXT("ItemManagerComponent"));
	ItemTargetingComponent = CreateDefaultSubobject<UItemTargetingComponent>(TEXT("ItemTargetingComponent"));
    
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

	if (KnifeActorClass)
	{
			KnifeActor = GetWorld()->SpawnActor<AKnifeActor>(KnifeActorClass);

			if (KnifeActor)
			{
					KnifeActor->AttachToCharacter(GetMesh(), TEXT("HandGrip_R"));
					KnifeActor->HideWeapon();
			}
	}

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

void AMyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
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
	if (ActiveTimedKnifeEvent && bTimedKnifeAttackInProgress)
	{
		ActiveTimedKnifeEvent->NotifyKillConfirmed();
	}
}

void AMyCharacter::NotifyTimedKnifeAttackSucceeded()
{
	if (ActiveTimedKnifeEvent)
	{
		ActiveTimedKnifeEvent->NotifyKillConfirmed();
	}
}

void AMyCharacter::RegisterActiveTimedKnifeEvent(UPersonalEventTimeKnife* EventInstance)
{
	if (ActiveTimedKnifeEvent && ActiveTimedKnifeEvent != EventInstance)
	{
		ActiveTimedKnifeEvent->CancelCountdown();
	}

	ActiveTimedKnifeEvent = EventInstance;
}

void AMyCharacter::ClearActiveTimedKnifeEvent(UPersonalEventTimeKnife* EventInstance)
{
	if (!EventInstance || ActiveTimedKnifeEvent == EventInstance)
	{
		ActiveTimedKnifeEvent = nullptr;
	}
}

void AMyCharacter::ForceDeadByPersonalEvent()
{
	HandleDead();
}

void AMyCharacter::SetTimedKnifeAttackInProgress(bool bInProgress)
{
	bTimedKnifeAttackInProgress = bInProgress;
}

void AMyCharacter::Multicast_ShowPublicMaliceAnnouncement_Implementation(const FString& PlayerName, int32 MaliceCount)
{
	if (AMyPlayerController* LocalPlayerController = Cast<AMyPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		LocalPlayerController->ShowPublicMaliceAnnouncement(PlayerName, MaliceCount);
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
				if (IMC_Default)
				{
					Subsystem->RemoveMappingContext(IMC_Default);
				}
				if (IMC_QTE)
				{
					Subsystem->AddMappingContext(IMC_QTE, 1);
				}

				if (GetCharacterMovement())
				{
					GetCharacterMovement()->StopMovementImmediately();
					GetCharacterMovement()->SetMovementMode(MOVE_None);
				}
			}
			else
			{
				if (IMC_QTE)
				{
					Subsystem->RemoveMappingContext(IMC_QTE);
				}
				if (IMC_Default)
				{
					Subsystem->AddMappingContext(IMC_Default, 0);
				}

				if (GetCharacterMovement())
				{
					GetCharacterMovement()->SetMovementMode(MOVE_Walking);
				}
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

	bIsDead = true;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->DisableMovement();
	}

	if (UMyAnimInstance* Anim = Cast<UMyAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		Anim->PlayDeathMontage();
	}

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
		MyPlayerController->SetItemTimerVisible(false);
	}

	if (IA_VoiceChat)
	{
		EIC->BindAction(IA_VoiceChat, ETriggerEvent::Started, this, &AMyCharacter::OnToggleVoiceChat);
	}

	if (IA_ESC)
	{
		EIC->BindAction(IA_ESC, ETriggerEvent::Started, this, &AMyCharacter::OnEscPressed);
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

bool AMyCharacter::HandleRewardPickup(AActor* InteractedActor, const URewardDefinition* RewardDefinition)
{
	if (!InteractedActor || !RewardDefinition)
	{
		return false;
	}

	ERewardCategory EffectiveCategory = RewardDefinition->RewardCategory;
	if (UClass* LogicClass = RewardDefinition->ResolveRewardLogicClass())
	{
		const bool bIsLegacyPersonalEventClass =
			LogicClass->IsChildOf(UItemTimeKnife::StaticClass());

		if (LogicClass->IsChildOf(UBasePersonalEvent::StaticClass()) || bIsLegacyPersonalEventClass)
		{
			EffectiveCategory = ERewardCategory::PersonalEvent;
		}
		else if (LogicClass->IsChildOf(UBaseGroupEvent::StaticClass()))
		{
			EffectiveCategory = ERewardCategory::GroupEvent;
		}
	}

	switch (EffectiveCategory)
	{
	case ERewardCategory::BasicItem:
	{
		const UItemDefinition* ItemDefinition = Cast<UItemDefinition>(const_cast<URewardDefinition*>(RewardDefinition));
		if (!ItemDefinition)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Reward] Basic item pickup failed: reward is not UItemDefinition. item=%s"), *GetNameSafe(RewardDefinition));
			return false;
		}
		return HandleBasicItemPickup(InteractedActor, ItemDefinition);
	}

	case ERewardCategory::PersonalEvent:
		return HandlePersonalEventPickup(InteractedActor, RewardDefinition);

	case ERewardCategory::GroupEvent:
		return HandleGroupEventPickup(InteractedActor, RewardDefinition);

	default:
		UE_LOG(LogTemp, Warning, TEXT("[Reward] Unknown category. item=%s"), *GetNameSafe(RewardDefinition));
		return false;
	}
}

bool AMyCharacter::HandleBasicItemPickup(AActor* InteractedActor, const UItemDefinition* RewardDefinition)
{
	(void)InteractedActor;

	if (!RewardDefinition || RewardDefinition->RewardCategory != ERewardCategory::BasicItem)
	{
		return false;
	}

	if (!ItemManagerComponent)
	{
		return false;
	}

	int32 AddedSlotIndex = INDEX_NONE;
	UItemDefinition* MutableDefinition = const_cast<UItemDefinition*>(RewardDefinition);
	if (!ItemManagerComponent->TryAddItemToFirstEmptySlot(MutableDefinition, 1, AddedSlotIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Reward] Basic item pickup failed: add failed or slot full. item=%s"), *GetNameSafe(RewardDefinition));
		return false;
	}

	UClass* LogicClass = RewardDefinition ? RewardDefinition->ResolveRewardLogicClass() : nullptr;
	const bool bIsShieldItem =
		RewardDefinition &&
		LogicClass &&
		LogicClass->IsChildOf(UItemShield::StaticClass());

	if (bIsShieldItem && CombatStatusComponent)
	{
		CombatStatusComponent->AddShield(FMath::Max(1, RewardDefinition->Payload.BlockCount));
	}

	return true;
}

bool AMyCharacter::HandlePersonalEventPickup(AActor* InteractedActor, const URewardDefinition* RewardDefinition)
{
	if (!RewardDefinition)
	{
		return false;
	}

	UClass* LogicClass = RewardDefinition->ResolveRewardLogicClass();
	UClass* PersonalEventClass = nullptr;
	if (LogicClass)
	{
		if (LogicClass->IsChildOf(UBasePersonalEvent::StaticClass()))
		{
			PersonalEventClass = LogicClass;
		}
		else if (LogicClass->IsChildOf(UItemTimeKnife::StaticClass()))
		{
			PersonalEventClass = UPersonalEventTimeKnife::StaticClass();
		}
	}

	// Fallback for assets with PersonalEvent category but missing/misaligned RewardLogicClass.
	if (!PersonalEventClass)
	{
		URewardDefinition* MutableRewardDefinition = const_cast<URewardDefinition*>(RewardDefinition);
		if (Cast<UPersonalEventTimeKnifeDefinition>(MutableRewardDefinition))
		{
			PersonalEventClass = UPersonalEventTimeKnife::StaticClass();
		}
		else if (Cast<UPersonalEventInspectMaliceDefinition>(MutableRewardDefinition))
		{
			PersonalEventClass = UPersonalEventInspectMalice::StaticClass();
		}
		else if (Cast<UPersonalEventPublicMaliceDefinition>(MutableRewardDefinition))
		{
			PersonalEventClass = UPersonalEventPublicMalice::StaticClass();
		}
		else if (Cast<UPersonalEventMaliceDefinition>(MutableRewardDefinition))
		{
			PersonalEventClass = UPersonalEventMalice::StaticClass();
		}
	}

	if (!PersonalEventClass || !PersonalEventClass->IsChildOf(UBasePersonalEvent::StaticClass()))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Reward] Personal event pickup failed: invalid logic class. item=%s logic=%s mapped=%s"),
			*GetNameSafe(RewardDefinition),
			*GetNameSafe(LogicClass),
			*GetNameSafe(PersonalEventClass)
		);
		return false;
	}

	UBasePersonalEvent* PersonalEvent = NewObject<UBasePersonalEvent>(this, PersonalEventClass);
	if (!PersonalEvent)
	{
		return false;
	}

	PersonalEvent->EventID = RewardDefinition->ItemId;
	PersonalEvent->InitializeContext(RewardDefinition, InteractedActor);
	PersonalEvent->ExecuteEvent(this);
	
	return true;
}

bool AMyCharacter::HandleGroupEventPickup(AActor* InteractedActor, const URewardDefinition* RewardDefinition)
{
	if (!RewardDefinition)
	{
		return false;
	}

	UClass* LogicClass = RewardDefinition->ResolveRewardLogicClass();
	if (!LogicClass || !LogicClass->IsChildOf(UBaseGroupEvent::StaticClass()))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Reward] Group event pickup failed: invalid logic class. item=%s logic=%s"),
			*GetNameSafe(RewardDefinition),
			*GetNameSafe(LogicClass)
		);
		return false;
	}

	UBaseGroupEvent* GroupEvent = NewObject<UBaseGroupEvent>(this, LogicClass);
	if (!GroupEvent)
	{
		return false;
	}

	GroupEvent->EventID = RewardDefinition->ItemId;
	GroupEvent->InitializeContext(RewardDefinition, InteractedActor);
	GroupEvent->ExecuteEvent(this);
	return true;
}

void AMyCharacter::InteractionCompleteResult()
{
	AActor* InteractedActor = InteractionComponent->GetLastInteractedActor();
	if (!InteractedActor)
	{
		return;
	}

	if (UMyAnimInstance* Anim = Cast<UMyAnimInstance>(GetMesh()->GetAnimInstance()))
	{
		Anim->PlayInteractMontage();
	}

	ABaseInteractable* Interactable = Cast<ABaseInteractable>(InteractedActor);
	if (!Interactable)
	{
		return;
	}

	Interactable->OnInteractionSuccess(this);

	if (!IsValid(Interactable) || Interactable->IsPendingKillPending())
	{
		return;
	}

	const URewardDefinition* RewardDefinition = Interactable->GetRewardDefinition();
	if (!RewardDefinition)
	{
		return;
	}

	if (HandleRewardPickup(InteractedActor, RewardDefinition))
	{
		InteractedActor->Destroy();
	}
}

void AMyCharacter::OnItemSelect(const FInputActionValue& Value)
{
	if (!QuickSlotComponent)
	{
		return;
	}

	int32 SlotNumberOneBased = INDEX_NONE;

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
		else if (PC->IsInputKeyDown(EKeys::Six) || PC->IsInputKeyDown(EKeys::NumPadSix))
		{
			SlotNumberOneBased = 6;
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

		UClass* UsedLogicClass = UsedItemDefinition ? UsedItemDefinition->ResolveRewardLogicClass() : nullptr;
		const bool bUsedTimedKillKnife = UsedLogicClass && UsedLogicClass->IsChildOf(UItemTimeKnife::StaticClass());

		if (UMyAnimInstance* Anim = Cast<UMyAnimInstance>(GetMesh()->GetAnimInstance()))
		{
			if (bUsedTimedKillKnife)
			{
				Anim->PlayTimeKnifeMontage();
			}
			else
			{
				Anim->PlayAttackMontage();
			}
		}
	}
}

void AMyCharacter::OnInteractHoldProgress(const FInputActionValue& Value)
{
	if (InteractionComponent)
	{
		const bool bIsComplete = InteractionComponent->HandleInteractHoldProgress(GetWorld()->GetDeltaSeconds());
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

void AMyCharacter::OnToggleVoiceChat(const FInputActionValue& Value)
{
	PrivateVoiceChatComponent = FindComponentByClass<UPrivateVoiceChatComponent>();
	if (PrivateVoiceChatComponent)
	{
		PrivateVoiceChatComponent->ToggleMicrophone();
	}
}

void AMyCharacter::OnEscPressed(const FInputActionValue& Value)
{
	if (AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(GetController()))
	{
		MyPlayerController->ToggleInGameSettingMenu();
	}
}

void AMyCharacter::OnQTEInput(const FInputActionValue& Value)
{
	if (!InteractionComponent)
	{
		return;
	}

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

// 무기 표시/숨김 함수 추가(애니메이션 재생 시 위치 참조용)
void AMyCharacter::ShowKnife()
{
    if (KnifeActor)
    {
        KnifeActor->ShowWeapon();
    }
}

void AMyCharacter::HideKnife()
{
    if (KnifeActor)
    {
        KnifeActor->HideWeapon();
    }
}
