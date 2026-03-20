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
	EquipmentComponent = CreateDefaultSubobject<UEquipmentComponent>(TEXT("EquipmentComponent"));
	CharacterRewardComponent = CreateDefaultSubobject<UCharacterRewardComponent>(TEXT("CharacterRewardComponent"));
	PlayerEventComponent = CreateDefaultSubobject<UPlayerEventComponent>(TEXT("PlayerEventComponent"));
	CharacterHealthComponent = CreateDefaultSubobject<UCharacterHealthComponent>(TEXT("CharacterHealthComponent"));
	CharacterActionInputComponent = CreateDefaultSubobject<UCharacterActionInputComponent>(TEXT("CharacterActionInputComponent"));
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

void AMyCharacter::BroadcastPublicMaliceAnnouncement(const FString& PlayerName, int32 MaliceCount)
{
	if (MaliceComponent)
	{
		MaliceComponent->BroadcastPublicMaliceAnnouncement(PlayerName, MaliceCount);
	}
}









