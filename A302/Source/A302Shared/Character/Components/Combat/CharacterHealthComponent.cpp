#include "Character/Components/Combat/CharacterHealthComponent.h"
#include "Character/MyCharacter.h"
#include "Character/Components/Inventory/QuickSlotComponent.h"
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

	UQuickSlotComponent* QuickSlotComponent = OwnerCharacter->FindComponentByClass<UQuickSlotComponent>();
	if (QuickSlotComponent && QuickSlotComponent->TryAutoUseItem())
	{
		LogAndScreenHealthMessage(TEXT("[MyCharacter] Blocked by auto-used shield"), FColor::Green, 1.5f);
		return 0.f;
	}

	HandleDead();

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
