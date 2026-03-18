#include "Character/MyCharacter.h"

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
#include "GameData/Events/PersonalEvents/PersonalEventCursedSwordDefinition.h"
#include "GameData/RewardDefinition.h"
#include "GameData/RewardTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GamePlay/Actor/WeaponActor.h"
#include "GamePlay/Events/GroupEvents/BaseGroupEvent.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"
#include "GamePlay/Items/BaseItem.h"
#include "GamePlay/Items/ItemShield.h"
#include "GamePlay/Items/ItemTimeKnife.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "Net/UnrealNetwork.h"
#include "Object/BaseInteractable.h"
#include "GameMode/A302PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Interface/A302ServerRewardBridge.h"
#include "Interface/A302ClientEventBridge.h"
#include "Interface/A302AnimationBridge.h"
#include "Interface/A302TimedKillEventBridge.h"
#include "A302GameplayGuards.h"

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

void AMyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AMyCharacter::HandleShieldChanged(int32 NewCount)
{
	if (IA302ClientEventBridge* ClientEventBridge = Cast<IA302ClientEventBridge>(GetController()))
	{
		ClientEventBridge->UpdateShieldCount(FMath::Max(0, NewCount));
	}
}

void AMyCharacter::HandleMaliceChanged(int32 NewCount)
{
	bMaliceEnding = NewCount >= 3;
	bNiceEnding = !bMaliceEnding;

	if (IA302ClientEventBridge* ClientEventBridge = Cast<IA302ClientEventBridge>(GetController()))
	{
		ClientEventBridge->UpdateMaliceCount(FMath::Max(0, NewCount));
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

	if (!A302GameplayGuards::IsGameplayEnabledCharacter(this))
	{
		return 0.f;
	}

	const APlayerState* VictimPlayerState = GetPlayerState();
	if (const APlayerState* InstigatorPlayerState = EventInstigator ? EventInstigator->PlayerState : nullptr)
	{
		if (!A302RoomScope::ArePlayersInSameActiveLogicalRoom(InstigatorPlayerState, VictimPlayerState))
		{
			UE_LOG(LogTemp, Verbose, TEXT("[MyCharacter] Ignore cross-room damage. victim=%s instigator=%s"), *GetNameSafe(this), *GetNameSafe(EventInstigator));
			return 0.f;
		}
	}

	if (QuickSlotComponent && QuickSlotComponent->TryAutoUseItem())
	{
		LogAndScreenCharacter(TEXT("[MyCharacter] Blocked by auto-used shield"), FColor::Green, 1.5f);
		return 0.f;
	}

	HandleDead();

	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

bool AMyCharacter::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	const APlayerController* ViewerPC = Cast<APlayerController>(RealViewer);
	if (!ViewerPC)
	{
		return Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
	}

	if (ViewerPC == GetController())
	{
		return true;
	}

	const APlayerState* ViewerState = ViewerPC->PlayerState;
	const APlayerState* CharacterState = GetPlayerState();
	if (ViewerState && CharacterState && !A302RoomScope::ArePlayersInSameActiveLogicalRoom(ViewerState, CharacterState))
	{
		return false;
	}

	return Super::IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
}

void AMyCharacter::NotifyKilledCharacter()
{
	if (ActiveTimedKnifeEventObject && bTimedKnifeAttackInProgress)
	{
		if (IA302TimedKillEventBridge* TimedKillBridge = Cast<IA302TimedKillEventBridge>(ActiveTimedKnifeEventObject))
		{
			TimedKillBridge->NotifyTimedKillConfirmed();
		}
	}
}

void AMyCharacter::NotifyTimedKnifeAttackSucceeded()
{
	if (IA302TimedKillEventBridge* TimedKillBridge = Cast<IA302TimedKillEventBridge>(ActiveTimedKnifeEventObject))
	{
		TimedKillBridge->NotifyTimedKillConfirmed();
	}
}

void AMyCharacter::RegisterTimedKillEvent(UObject* EventInstance)
{
	if (IA302TimedKillEventBridge* ExistingTimedKillBridge = Cast<IA302TimedKillEventBridge>(ActiveTimedKnifeEventObject);
		ExistingTimedKillBridge && ActiveTimedKnifeEventObject != EventInstance)
	{
		ExistingTimedKillBridge->CancelTimedKillCountdown();
	}

	ActiveTimedKnifeEventObject = EventInstance;
}

void AMyCharacter::ClearTimedKillEvent(UObject* EventInstance)
{
	if (!EventInstance || ActiveTimedKnifeEventObject == EventInstance)
	{
		ActiveTimedKnifeEventObject = nullptr;
	}
}

void AMyCharacter::ForceDeathByPersonalEvent()
{
	HandleDead();
}

void AMyCharacter::SetTimedKnifeAttackInProgress(bool bInProgress)
{
	bTimedKnifeAttackInProgress = bInProgress;
}

void AMyCharacter::EquipKnifeWeapon()
{
	if (KnifeActorClass)
	{
		EquipWeapon(KnifeActorClass);
	}
}

void AMyCharacter::EquipTimeKnifeWeapon()
{
	if (TimeKnifeActorClass)
	{
		EquipWeapon(TimeKnifeActorClass);
	}
}

void AMyCharacter::EquipShieldWeapon()
{
	if (ShieldActorClass)
	{
		EquipWeapon(ShieldActorClass);
	}
}

void AMyCharacter::PlayShieldBlockPresentation()
{
	if (IA302AnimationBridge* Anim = Cast<IA302AnimationBridge>(GetMesh()->GetAnimInstance()))
	{
		Anim->PlayBlockAnimation();
	}
}

void AMyCharacter::Multicast_ShowPublicMaliceAnnouncement_Implementation(const FString& PlayerName, int32 MaliceCount)
{
	if (IA302ClientEventBridge* ClientEventBridge = Cast<IA302ClientEventBridge>(UGameplayStatics::GetPlayerController(this, 0)))
	{
		ClientEventBridge->ShowPublicMaliceAnnouncement(PlayerName, MaliceCount);
	}
}

void AMyCharacter::BroadcastPublicMaliceAnnouncement(const FString& PlayerName, int32 MaliceCount)
{
	if (HasAuthority())
	{
		Multicast_ShowPublicMaliceAnnouncement(PlayerName, MaliceCount);
		return;
	}

	if (IA302ClientEventBridge* ClientEventBridge = Cast<IA302ClientEventBridge>(GetController()))
	{
		ClientEventBridge->ShowPublicMaliceAnnouncement(PlayerName, MaliceCount);
	}
}

void AMyCharacter::SetQTEInputMode(bool bIsQTE)
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetIgnoreMoveInput(bIsQTE);

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

				// MOVE_None/MOVE_Walking 강제 전환은 서버-클라 이동 예측 불일치를 키울 수 있어 생략.
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

	if (HasAuthority())
	{
		if (AA302PlayerState* A302PlayerState = GetPlayerState<AA302PlayerState>())
		{
			A302PlayerState->bIsAlive = false;
		}
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->DisableMovement();
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetIgnoreMoveInput(true);
	}

	if (IA302AnimationBridge* Anim = Cast<IA302AnimationBridge>(GetMesh()->GetAnimInstance()))
	{
		Anim->PlayDeathAnimation();
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

	if (IA302ClientEventBridge* ClientEventBridge = Cast<IA302ClientEventBridge>(GetController()))
	{
		ClientEventBridge->SetItemTimerVisibleForClient(false);
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
	if (bIsDead)
	{
		return;
	}

	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->IsMoveInputIgnored())
		{
			return;
		}
	}

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
	float LookSensitivityMultiplier = 1.0f;
	if (const AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(GetController()))
	{
		LookSensitivityMultiplier = MyPlayerController->GetMouseSensitivityMultiplier();
	}

	AddControllerYawInput(Axis.X * LookSensitivityMultiplier);
	AddControllerPitchInput(Axis.Y * LookSensitivityMultiplier);
}

void AMyCharacter::OnJump(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}

	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (PC->IsMoveInputIgnored())
		{
			return;
		}
	}

	Jump();
}

void AMyCharacter::OnJumpReleased(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}

	StopJumping();
}

bool AMyCharacter::HandleRewardPickup(AActor* InteractedActor, const URewardDefinition* RewardDefinition)
{
	if (!A302GameplayGuards::IsGameplayEnabledCharacter(this))
	{
		return false;
	}

	if (!RewardDefinition)
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

bool AMyCharacter::ShouldGrantRewardLocally(const URewardDefinition* RewardDefinition) const
{
	if (!RewardDefinition)
	{
		return false;
	}

	if (RewardDefinition->RewardCategory == ERewardCategory::BasicItem)
	{
		return true;
	}

	UClass* LogicClass = RewardDefinition->ResolveRewardLogicClass();
	if (LogicClass && LogicClass->IsChildOf(UItemTimeKnife::StaticClass()))
	{
		return true;
	}

	URewardDefinition* MutableRewardDefinition = const_cast<URewardDefinition*>(RewardDefinition);
	return
		Cast<UPersonalEventInspectMaliceDefinition>(MutableRewardDefinition) != nullptr ||
		Cast<UPersonalEventCursedSwordDefinition>(MutableRewardDefinition) != nullptr;
}

void AMyCharacter::ResolveInteractionRewardOnServer(ABaseInteractable* Interactable)
{
	if (!HasAuthority() || !IsValid(Interactable))
	{
		return;
	}

	if (!Interactable->TryConsumeInteraction())
	{
		return;
	}

	const URewardDefinition* RewardDefinition = Interactable->GetRewardDefinition();
	if (RewardDefinition)
	{
		if (ShouldGrantRewardLocally(RewardDefinition))
		{
			Client_GrantInteractionReward(const_cast<URewardDefinition*>(RewardDefinition));
		}
		else
		{
			HandleRewardPickup(Interactable, RewardDefinition);
		}
	}

	Interactable->ForceNetUpdate();
	Interactable->Destroy();
}

void AMyCharacter::Server_RequestInteractionReward_Implementation(ABaseInteractable* Interactable)
{
	if (!HasAuthority() || !IsValid(Interactable))
	{
		return;
	}

	if (!A302GameplayGuards::IsGameplayEnabledCharacter(this))
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Interaction] Server rejected reward: gameplay not enabled for player=%s"), *GetNameSafe(this));
		return;
	}

	const float MaxAllowedDistance = FMath::Max(InteractionDistance, 0.f) + 200.f;
	if (GetDistanceTo(Interactable) > MaxAllowedDistance)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Interaction] Server rejected interactable reward: too far. player=%s actor=%s"),
			*GetName(),
			*GetNameSafe(Interactable)
		);
		return;
	}

	ResolveInteractionRewardOnServer(Interactable);
}

void AMyCharacter::Client_GrantInteractionReward_Implementation(URewardDefinition* RewardDefinition)
{
	HandleRewardPickup(nullptr, RewardDefinition);
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
	if (ItemManagerComponent->TryAddItemToFirstEmptySlot(MutableDefinition, 1, AddedSlotIndex))
	{
		UClass* LogicClass = RewardDefinition->ResolveRewardLogicClass();
		
		if (LogicClass && LogicClass->IsChildOf(UBaseItem::StaticClass()))
		{
			if (const UBaseItem* BaseItemLogic = Cast<UBaseItem>(LogicClass->GetDefaultObject()))
			{
				BaseItemLogic->OnItemAcquired(this);
			}
		}
		
		const bool bIsShieldItem = RewardDefinition && LogicClass && LogicClass->IsChildOf(UItemShield::StaticClass());
		if (bIsShieldItem && CombatStatusComponent)
		{
			CombatStatusComponent->AddShield(FMath::Max(1, RewardDefinition->Payload.BlockCount));
		}

		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Reward] Basic item pickup failed: add failed or slot full. item=%s"), *GetNameSafe(RewardDefinition));
	return false;
}

bool AMyCharacter::HandlePersonalEventPickup(AActor* InteractedActor, const URewardDefinition* RewardDefinition)
{
	if (!RewardDefinition)
	{
		return false;
	}

	UClass* LogicClass = RewardDefinition->ResolveRewardLogicClass();
	const bool bIsPersonalEventClass = LogicClass && LogicClass->IsChildOf(UBasePersonalEvent::StaticClass());
	const bool bIsLegacyTimedKnifeLogic = LogicClass && LogicClass->IsChildOf(UItemTimeKnife::StaticClass());
	const bool bIsCursedSwordDefinition =
		Cast<UPersonalEventCursedSwordDefinition>(const_cast<URewardDefinition*>(RewardDefinition)) != nullptr;

	if (!bIsPersonalEventClass && !bIsLegacyTimedKnifeLogic && !bIsCursedSwordDefinition)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[Event] Invalid personal event reward logic. item=%s class=%s"),
			*GetNameSafe(RewardDefinition),
			*GetNameSafe(LogicClass)
		);
		return false;
	}

	if (IA302ServerRewardBridge* RewardBridge = Cast<IA302ServerRewardBridge>(GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr))
	{
		return RewardBridge->TryHandlePersonalEventReward(this, InteractedActor, RewardDefinition);
	}

	return false;
}

bool AMyCharacter::HandleGroupEventPickup(AActor* InteractedActor, const URewardDefinition* RewardDefinition)
{
	if (IA302ServerRewardBridge* RewardBridge = Cast<IA302ServerRewardBridge>(GetWorld() ? GetWorld()->GetAuthGameMode() : nullptr))
	{
		return RewardBridge->TryHandleGroupEventReward(this, InteractedActor, RewardDefinition);
	}

	return false;
}

void AMyCharacter::InteractionCompleteResult()
{
	AActor* InteractedActor = InteractionComponent->GetLastInteractedActor();
	if (!InteractedActor)
	{
		return;
	}

	if (IA302AnimationBridge* Anim = Cast<IA302AnimationBridge>(GetMesh()->GetAnimInstance()))
	{
		Anim->PlayInteractAnimation();
	}

	ABaseInteractable* Interactable = Cast<ABaseInteractable>(InteractedActor);
	if (!IsValid(Interactable))
	{
		return;
	}

	Interactable->OnInteractionSuccess(this);

	if (HasAuthority())
	{
		ResolveInteractionRewardOnServer(Interactable);
	}
	else
	{
		Server_RequestInteractionReward(Interactable);
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
		if (UsedLogicClass && UsedLogicClass->IsChildOf(UBaseItem::StaticClass()))
		{
			if (const UBaseItem* BaseItemLogic = Cast<UBaseItem>(UsedLogicClass->GetDefaultObject()))
			{
				BaseItemLogic->OnItemUsed(this);
			}
		}
		const bool bUsedTimedKillKnife = UsedLogicClass && UsedLogicClass->IsChildOf(UItemTimeKnife::StaticClass());

		if (IA302AnimationBridge* Anim = Cast<IA302AnimationBridge>(GetMesh()->GetAnimInstance()))
		{
			if (bUsedTimedKillKnife)
			{
				EquipWeapon(TimeKnifeActorClass);
				Anim->PlayTimeKnifeAnimation();
			}
			else
			{
				EquipWeapon(KnifeActorClass);
				Anim->PlayAttackAnimation();
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
	if (IA302ClientEventBridge* ClientEventBridge = Cast<IA302ClientEventBridge>(GetController()))
	{
		ClientEventBridge->ToggleVoiceChatCapture();
	}
}

void AMyCharacter::OnEscPressed(const FInputActionValue& Value)
{
	if (IA302ClientEventBridge* ClientEventBridge = Cast<IA302ClientEventBridge>(GetController()))
	{
		ClientEventBridge->ToggleInGameSettingMenu();
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

// 무기 숨김 함수 (애니메이션 재생 시 위치 참조용)
void AMyCharacter::EquipWeapon(TSubclassOf<AWeaponActor> WeaponClass)
{
    UE_LOG(LogTemp, Warning, TEXT("EquipWeapon called"));

    if (!WeaponClass)
    {
        UE_LOG(LogTemp, Error, TEXT("WeaponClass is NULL"));
        return;
    }

    // 기존 무기 제거
    if (CurrentWeaponActor)
    {
        CurrentWeaponActor->Destroy();
        CurrentWeaponActor = nullptr;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("World is NULL"));
        return;
    }

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.Instigator = this;

    // 무기 Spawn
    CurrentWeaponActor = World->SpawnActor<AWeaponActor>(
        WeaponClass,
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        Params
    );

    if (!CurrentWeaponActor)
    {
        UE_LOG(LogTemp, Error, TEXT("Weapon Spawn FAILED"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Weapon Spawn SUCCESS"));

    // 캐릭터 손 소켓에 부착
    FName SocketName = TEXT("HandGrip_R");

		if (WeaponClass == ShieldActorClass)
		{
				SocketName = TEXT("HandGrip_L");
		}

		CurrentWeaponActor->AttachToComponent(
				GetMesh(),
				FAttachmentTransformRules::SnapToTargetIncludingScale,
				SocketName
		);

    // 무기 표시
    CurrentWeaponActor->ShowWeapon();
}

void AMyCharacter::ShowWeapon()
{
		UE_LOG(LogTemp, Warning, TEXT("ShowWeapon called"));
    if (CurrentWeaponActor)
    {
        CurrentWeaponActor->ShowWeapon();
    }
}

void AMyCharacter::HideWeapon()
{
    if (CurrentWeaponActor)
    {
        CurrentWeaponActor->HideWeapon();
    }
}
