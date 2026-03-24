#include "Character/Components/Combat/CharacterHealthComponent.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "Character/Components/Combat/CombatStatusComponent.h"
#include "Character/Components/Inventory/QuickSlotComponent.h"
#include "GameData/Items/ItemDefinition.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameMode/A302PlayerState.h"
#include "Interface/A302AnimationBridge.h"
#include "A302GameplayGuards.h"
#include "Engine/Engine.h"

UCharacterHealthComponent::UCharacterHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bIsDead = false;
}

void UCharacterHealthComponent::BeginPlay()
{
	Super::BeginPlay();
}

AMyCharacter* UCharacterHealthComponent::GetOwnerCharacter() const
{
	return Cast<AMyCharacter>(GetOwner());
}

void UCharacterHealthComponent::LogAndScreenHealthMessage(const FString& Message, const FColor& Color, float Duration) const
{
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, Duration, Color, Message);
	}
}

float UCharacterHealthComponent::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter) return 0.f;

	// Damage resolution must be authoritative to avoid client-only death desync.
	if (!OwnerCharacter->HasAuthority())
	{
		return 0.f;
	}

	if (bIsDead)
	{
		return 0.f;
	}

	if (!A302GameplayGuards::IsGameplayEnabledCharacter(OwnerCharacter))
	{
		return 0.f;
	}

	const APlayerState* VictimPlayerState = OwnerCharacter->GetPlayerState();
	if (const APlayerState* InstigatorPlayerState = EventInstigator ? EventInstigator->PlayerState : nullptr)
	{
		if (!A302RoomScope::ArePlayersInSameActiveLogicalRoom(InstigatorPlayerState, VictimPlayerState))
		{
			UE_LOG(LogTemp, Verbose, TEXT("[MyCharacter] Ignore cross-room damage. victim=%s instigator=%s"), *GetNameSafe(OwnerCharacter), *GetNameSafe(EventInstigator));
			return 0.f;
		}
	}

	UCombatStatusComponent* CombatStatusComponent = OwnerCharacter->FindComponentByClass<UCombatStatusComponent>();
	const int32 PreviousShieldCount = CombatStatusComponent ? CombatStatusComponent->ShieldBlockCount : 0;

	UQuickSlotComponent* QuickSlotComponent = OwnerCharacter->FindComponentByClass<UQuickSlotComponent>();
	UItemDefinition* AutoUsedItemDefinition = nullptr;
	int32 AutoUsedSlotIndex = INDEX_NONE;
	bool bAutoUseBecameEmpty = false;
	if (QuickSlotComponent && QuickSlotComponent->TryAutoUseItem(&AutoUsedItemDefinition, &AutoUsedSlotIndex, &bAutoUseBecameEmpty))
	{
		// In some pickup/use paths, auto-use can succeed while shield count replication lags.
		// Ensure the defended hit always consumes one shield stack on authority.
		if (CombatStatusComponent &&
			PreviousShieldCount > 0 &&
			CombatStatusComponent->ShieldBlockCount >= PreviousShieldCount)
		{
			CombatStatusComponent->TryConsumeShieldToBlock();
		}

		if (bAutoUseBecameEmpty && AutoUsedSlotIndex != INDEX_NONE)
		{
			if (AMyPlayerController* OwnerPlayerController = Cast<AMyPlayerController>(OwnerCharacter->GetController()))
			{
				const FName ExpectedItemId = AutoUsedItemDefinition ? AutoUsedItemDefinition->ItemId : NAME_None;
				OwnerPlayerController->Client_RemoveQuickSlotItemByServer(AutoUsedSlotIndex, ExpectedItemId);
			}
		}

		LogAndScreenHealthMessage(TEXT("[MyCharacter] Blocked by auto-used shield"), FColor::Green, 1.5f);
		return 0.f;
	}

	HandleDead();

	// Timed kill events (e.g., cursed sword) are confirmed only when an actual kill happens.
	AMyCharacter* KillerCharacter = EventInstigator ? Cast<AMyCharacter>(EventInstigator->GetCharacter()) : nullptr;
	if (!KillerCharacter && DamageCauser)
	{
		KillerCharacter = Cast<AMyCharacter>(DamageCauser);
	}
	if (!KillerCharacter && DamageCauser)
	{
		KillerCharacter = Cast<AMyCharacter>(DamageCauser->GetOwner());
	}
	if (KillerCharacter)
	{
		if (KillerCharacter != OwnerCharacter)
		{
			KillerCharacter->NotifyKilledCharacter();
		}
	}

	// Note: We bypass Super::TakeDamage since UActorComponent doesn't have it.
	// In the original, AMyCharacter called Super::TakeDamage, which affects ACharacter and AActor base processing.
	// Here, we're returning the original requested damage amount indicating application.
	// If you want actual engine damage to apply, we might need OwnerCharacter->Super::TakeDamage mapping.
	// Since this game is mostly 1-hit kill custom logic, we return DamageAmount here.
	return DamageAmount;
}

void UCharacterHealthComponent::HandleDead()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;

	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter) return;

	if (OwnerCharacter->HasAuthority())
	{
		if (AA302PlayerState* A302PlayerState = OwnerCharacter->GetPlayerState<AA302PlayerState>())
		{
			A302PlayerState->bIsAlive = false;
		}
	}

	if (OwnerCharacter->GetCharacterMovement())
	{
		OwnerCharacter->GetCharacterMovement()->DisableMovement();
	}

	if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
	{
		PC->SetIgnoreMoveInput(true);
	}

	if (IA302AnimationBridge* Anim = Cast<IA302AnimationBridge>(OwnerCharacter->GetMesh()->GetAnimInstance()))
	{
		Anim->PlayDeathAnimation();
	}

	LogAndScreenHealthMessage(TEXT("[MyCharacter] Dead"), FColor::Red, 4.0f);
}

void UCharacterHealthComponent::ForceDeathByPersonalEvent()
{
	HandleDead();
}
