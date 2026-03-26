#include "Character/MyCharacter.h"

#include "Character/Components/Combat/CombatStatusComponent.h"
#include "Character/Components/Interaction/InteractComponent.h"
#include "Character/Components/Inventory/ItemManagerComponent.h"
#include "Character/Components/Inventory/ItemTargetingComponent.h"
#include "Character/Components/KnifeAutoTestComponent.h"
#include "Character/Components/MaliceComponent.h"
#include "Character/Components/Inventory/QuickSlotComponent.h"
#include "Character/Components/Combat/EquipmentComponent.h"
#include "Character/Components/Interaction/CharacterRewardComponent.h"
#include "Character/Components/Combat/CharacterHealthComponent.h"
#include "Character/Components/Interaction/CharacterActionInputComponent.h"
#include "Character/Components/Interaction/InteractComponent.h"
#include "Character/MyPlayerController.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"	
#include "GameData/Items/ItemDefinition.h"
#include "GameData/Events/PersonalEvents/Malice/PersonalEventInspectMaliceDefinition.h"
#include "GameData/Events/PersonalEvents/Malice/PersonalEventMaliceDefinition.h"
#include "GameData/Events/PersonalEvents/Malice/PersonalEventPublicMaliceDefinition.h"
#include "GameData/Events/PersonalEvents/Equipment/PersonalEventCursedSwordDefinition.h"
#include "GameData/Events/PersonalEvents/Status/PersonalEventDevilsEyeDefinition.h"
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
#include "GamePlay/Items/ItemCursedSword.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "Net/UnrealNetwork.h"
#include "Object/BaseInteractable.h"
#include "GameMode/A302PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Interface/A302ServerRewardBridge.h"
#include "Character/MyPlayerController.h"
#include "Interface/A302AnimationBridge.h"
#include "Interface/A302TimedKillEventBridge.h"
#include "A302GameplayGuards.h"
#include "Character/Components/PlayerEventComponent.h"
#include "GameFramework/SpringArmComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyInput, Log, All);

namespace
{
	constexpr float EscapeSequenceDelaySeconds = 1.0f;

	void LogAndScreenCharacter(const FString& Message, const FColor& Color = FColor::Yellow, const float Duration = 3.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, Duration, Color, Message);
		}
	}

	bool HasEscapePointTag(const AActor* Actor)
	{
		if (!Actor)
		{
			return false;
		}

		static const FName EscapePointTag(TEXT("EscapePoint"));
		static const FName Stage3EscapePointTag(TEXT("Stage3EscapePoint"));
		return Actor->ActorHasTag(EscapePointTag) || Actor->ActorHasTag(Stage3EscapePointTag);
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
	InteractComp = InteractionComponent;
	InteractionQuickSlotComponent = InteractionComponent;
	QuickSlotComponent = CreateDefaultSubobject<UQuickSlotComponent>(TEXT("QuickSlotComponent"));
	CombatStatusComponent = CreateDefaultSubobject<UCombatStatusComponent>(TEXT("CombatStatusComponent"));
	MaliceComponent = CreateDefaultSubobject<UMaliceComponent>(TEXT("MaliceComponent"));
	KnifeAutoTestComponent = CreateDefaultSubobject<UKnifeAutoTestComponent>(TEXT("KnifeAutoTestComponent"));
	EquipmentComponent = CreateDefaultSubobject<UEquipmentComponent>(TEXT("EquipmentComponent"));
	CharacterRewardComponent = CreateDefaultSubobject<UCharacterRewardComponent>(TEXT("CharacterRewardComponent"));
	PlayerEventComponent = CreateDefaultSubobject<UPlayerEventComponent>(TEXT("PlayerEventComponent"));
	CharacterHealthComponent = CreateDefaultSubobject<UCharacterHealthComponent>(TEXT("CharacterHealthComponent"));
	CharacterActionInputComponent = CreateDefaultSubobject<UCharacterActionInputComponent>(TEXT("CharacterActionInputComponent"));
}

void AMyCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	ApplyCameraViewMode();
}

void AMyCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();
	ApplyCameraViewMode();
}

void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UCapsuleComponent* CollisionCapsule = GetCapsuleComponent())
	{
		CollisionCapsule->OnComponentBeginOverlap.RemoveDynamic(this, &AMyCharacter::HandleCapsuleBeginOverlap);
		CollisionCapsule->OnComponentBeginOverlap.AddDynamic(this, &AMyCharacter::HandleCapsuleBeginOverlap);
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

	ApplyCameraViewMode();
}

void AMyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AMyCharacter::HandleShieldChanged(int32 NewCount)
{
	if (AMyPlayerController* ClientEventBridge = Cast<AMyPlayerController>(GetController()))
	{
		ClientEventBridge->UpdateShieldCount(FMath::Max(0, NewCount));
	}
}

void AMyCharacter::HandleMaliceChanged(int32 NewCount)
{
	if (MaliceComponent)
	{
		MaliceComponent->bMaliceEnding = NewCount >= 3;
		MaliceComponent->bNiceEnding = !MaliceComponent->bMaliceEnding;
	}

	if (AMyPlayerController* ClientEventBridge = Cast<AMyPlayerController>(GetController()))
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
	if (CharacterHealthComponent)
	{
		float ActualDamage = CharacterHealthComponent->TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
		return Super::TakeDamage(ActualDamage, DamageEvent, EventInstigator, DamageCauser);
	}

	return 0.f;
}

bool AMyCharacter::IsDead() const
{
	if (const AA302PlayerState* A302PlayerState = GetPlayerState<AA302PlayerState>())
	{
		if (!A302PlayerState->bIsAlive)
		{
			return true;
		}
	}

	if (CharacterHealthComponent)
	{
		return CharacterHealthComponent->IsDead();
	}
	return false;
}

float AMyCharacter::GetInteractionProgressRatio() const
{
	return InteractionComponent ? InteractionComponent->GetInteractionProgressRatio() : 0.0f;
}

UInteractComponent* AMyCharacter::GetInteractionComponentCompat() const
{
	return InteractionComponent;
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

void AMyCharacter::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	TryHandleEscapePortalOverlap(OtherActor);
}

void AMyCharacter::HandleCapsuleBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	TryHandleEscapePortalOverlap(OtherActor);
}

bool AMyCharacter::TryHandleEscapePortalOverlap(AActor* OtherActor)
{
	if (!HasAuthority() || !HasEscapePointTag(OtherActor))
	{
		return false;
	}

	// 포탈 접근 제한은 EscapeRouteBlocker가 물리적으로 처리하므로 여기서는 별도 페이즈 체크 불필요
	AA302PlayerState* A302PlayerState = GetPlayerState<AA302PlayerState>();
	if (!A302PlayerState || A302PlayerState->bIsEscaped || !A302PlayerState->bIsAlive || !A302PlayerState->bGameplayEnabled)
	{
		return false;
	}

	A302PlayerState->SetEscaped(true);
	BeginEscapedSequence();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[EscapePoint] Player escaped by overlap. player=%s portal=%s room=%s"),
		*GetNameSafe(this),
		*GetNameSafe(OtherActor),
		*A302PlayerState->GetRoomCode()
	);

	return true;
}

void AMyCharacter::BeginEscapedSequence()
{
	if (bEscapedSequenceStarted)
	{
		return;
	}

	bEscapedSequenceStarted = true;

	UWorld* World = GetWorld();
	if (!World)
	{
		FinalizeEscapedSequence();
		return;
	}

	World->GetTimerManager().ClearTimer(EscapedSequenceTimerHandle);
	World->GetTimerManager().SetTimer(
		EscapedSequenceTimerHandle,
		this,
		&AMyCharacter::FinalizeEscapedSequence,
		EscapeSequenceDelaySeconds,
		false
	);
}

void AMyCharacter::FinalizeEscapedSequence()
{
	HandleEscapedState();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(EscapedSequenceTimerHandle);
	}
}

void AMyCharacter::HandleEscapedState()
{
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->SetIgnoreMoveInput(true);
		PC->SetIgnoreLookInput(true);
		DisableInput(PC);
	}

	ForceNetUpdate();
}

void AMyCharacter::ForceDeathByPersonalEvent()
{
	if (CharacterHealthComponent)
	{
		CharacterHealthComponent->ForceDeathByPersonalEvent();
	}
}

void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (CharacterActionInputComponent)
	{
		CharacterActionInputComponent->SetupPlayerInputComponent(PlayerInputComponent);
	}

	if (CombatStatusComponent)
	{
		HandleShieldChanged(CombatStatusComponent->ShieldBlockCount);
	}

	if (AMyPlayerController* ClientEventBridge = Cast<AMyPlayerController>(GetController()))
	{
		ClientEventBridge->SetItemTimerVisibleForClient(false);
	}
}

void AMyCharacter::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	Super::CalcCamera(DeltaTime, OutResult);

	if (IsLocallyControlled())
	{
		return;
	}

	UCameraComponent* TargetCamera = ResolveCameraForMode(EA302CameraViewMode::ThirdPerson);
	if (!TargetCamera)
	{
		TArray<UCameraComponent*> Cameras;
		GetComponents<UCameraComponent>(Cameras);
		TargetCamera = Cameras.Num() > 0 ? Cameras[0] : nullptr;
	}

	if (!TargetCamera)
	{
		return;
	}

	const FRotator SpectateViewRotation = GetBaseAimRotation();
	if (USpringArmComponent* ThirdPersonSpringArm = Cast<USpringArmComponent>(TargetCamera->GetAttachParent()))
	{
		const FVector ArmOrigin =
			ThirdPersonSpringArm->GetComponentLocation()
			+ SpectateViewRotation.RotateVector(ThirdPersonSpringArm->TargetOffset);
		const FVector CameraLocation =
			ArmOrigin
			- SpectateViewRotation.Vector() * ThirdPersonSpringArm->TargetArmLength
			+ SpectateViewRotation.RotateVector(ThirdPersonSpringArm->SocketOffset);

		OutResult.Location = CameraLocation;
		OutResult.Rotation = SpectateViewRotation;
		OutResult.FOV = TargetCamera->FieldOfView;
		return;
	}

	OutResult.Location = TargetCamera->GetComponentLocation();
	OutResult.Rotation = SpectateViewRotation;
	OutResult.FOV = TargetCamera->FieldOfView;
}

void AMyCharacter::NotifyKilledCharacter()
{
	if (PlayerEventComponent)
	{
		PlayerEventComponent->NotifyKilledCharacter();
	}
}

void AMyCharacter::NotifyTimedKnifeAttackSucceeded()
{
	if (PlayerEventComponent)
	{
		PlayerEventComponent->NotifyTimedKnifeAttackSucceeded();
	}
}

void AMyCharacter::RegisterTimedKillEvent(UObject* EventInstance)
{
	if (PlayerEventComponent)
	{
		PlayerEventComponent->RegisterTimedKillEvent(EventInstance);
	}
}

void AMyCharacter::ClearTimedKillEvent(UObject* EventInstance)
{
	if (PlayerEventComponent)
	{
		PlayerEventComponent->ClearTimedKillEvent(EventInstance);
	}
}

void AMyCharacter::SetTimedKnifeAttackInProgress(bool bInProgress)
{
	if (PlayerEventComponent)
	{
		PlayerEventComponent->SetTimedKnifeAttackInProgress(bInProgress);
	}
}

void AMyCharacter::SetCameraViewMode(EA302CameraViewMode NewMode)
{
	CameraViewMode = NewMode;
	ApplyCameraViewMode();
}

UCameraComponent* AMyCharacter::ResolveCameraForMode(EA302CameraViewMode Mode) const
{
	TArray<UCameraComponent*> Cameras;
	GetComponents<UCameraComponent>(Cameras);
	if (Cameras.Num() == 0)
	{
		return nullptr;
	}

	auto NameMatches = [](const UCameraComponent* CameraComponent, const FName& TargetName) -> bool
	{
		if (!CameraComponent || TargetName.IsNone())
		{
			return false;
		}
		return CameraComponent->GetFName() == TargetName;
	};

	for (UCameraComponent* CameraComponent : Cameras)
	{
		if (Mode == EA302CameraViewMode::ThirdPerson && NameMatches(CameraComponent, ThirdPersonCameraComponentName))
		{
			return CameraComponent;
		}
		if (Mode == EA302CameraViewMode::FirstPersonChest && NameMatches(CameraComponent, FirstPersonCameraComponentName))
		{
			return CameraComponent;
		}
	}

	auto IsThirdPersonCandidate = [](const UCameraComponent* CameraComponent) -> bool
	{
		if (!CameraComponent)
		{
			return false;
		}

		const FString ComponentName = CameraComponent->GetName();
		if (ComponentName.Contains(TEXT("Third"), ESearchCase::IgnoreCase)
			|| ComponentName.Contains(TEXT("Follow"), ESearchCase::IgnoreCase)
			|| ComponentName.Contains(TEXT("TP"), ESearchCase::IgnoreCase))
		{
			return true;
		}

		return CameraComponent->GetAttachParent()
			&& CameraComponent->GetAttachParent()->IsA<USpringArmComponent>();
	};

	auto IsFirstPersonCandidate = [](const UCameraComponent* CameraComponent) -> bool
	{
		if (!CameraComponent)
		{
			return false;
		}

		const FString ComponentName = CameraComponent->GetName();
		return ComponentName.Contains(TEXT("First"), ESearchCase::IgnoreCase)
			|| ComponentName.Contains(TEXT("Chest"), ESearchCase::IgnoreCase)
			|| ComponentName.Contains(TEXT("FP"), ESearchCase::IgnoreCase);
	};

	for (UCameraComponent* CameraComponent : Cameras)
	{
		if (Mode == EA302CameraViewMode::ThirdPerson && IsThirdPersonCandidate(CameraComponent))
		{
			return CameraComponent;
		}
		if (Mode == EA302CameraViewMode::FirstPersonChest && IsFirstPersonCandidate(CameraComponent))
		{
			return CameraComponent;
		}
	}

	if (Mode == EA302CameraViewMode::ThirdPerson)
	{
		return Cameras[0];
	}

	for (UCameraComponent* CameraComponent : Cameras)
	{
		if (!IsThirdPersonCandidate(CameraComponent))
		{
			return CameraComponent;
		}
	}

	return Cameras[0];
}

void AMyCharacter::ApplyCameraViewMode()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	TArray<UCameraComponent*> Cameras;
	GetComponents<UCameraComponent>(Cameras);
	if (Cameras.Num() == 0)
	{
		return;
	}

	UCameraComponent* TargetCamera = ResolveCameraForMode(CameraViewMode);
	if (!TargetCamera)
	{
		return;
	}

	for (UCameraComponent* CameraComponent : Cameras)
	{
		if (CameraComponent)
		{
			CameraComponent->SetActive(CameraComponent == TargetCamera);
		}
	}

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		PlayerController->SetViewTarget(this);
	}
}

void AMyCharacter::BroadcastPublicMaliceAnnouncement(const FString& PlayerName, int32 MaliceCount)
{
	if (MaliceComponent)
	{
		MaliceComponent->BroadcastPublicMaliceAnnouncement(PlayerName, MaliceCount);
	}
}








