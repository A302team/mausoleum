#include "Animation/MyAnimInstance.h"
#include "Character/MyCharacter.h"
#include "Character/Components/CombatStatusComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

void UMyAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    CachedCharacter = Cast<AMyCharacter>(TryGetPawnOwner());

    if (CachedCharacter)
    {
        CachedCombatComponent = CachedCharacter->FindComponentByClass<UCombatStatusComponent>();

        if (CachedCombatComponent)
        {
            PreviousShieldCount = CachedCombatComponent->ShieldBlockCount;
        }
    }
}

void UMyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    if (!CachedCharacter)
    {
        CachedCharacter = Cast<AMyCharacter>(TryGetPawnOwner());
    }

    if (!CachedCharacter) return;

    FVector Velocity = CachedCharacter->GetVelocity();
    Velocity.Z = 0.f;

    Speed = Velocity.Size();

    bIsInAir = CachedCharacter->GetCharacterMovement()->IsFalling();

    FVector Forward = CachedCharacter->GetActorForwardVector();
    FVector Right = CachedCharacter->GetActorRightVector();

    FVector NormalizedVel = Velocity.GetSafeNormal();

    float ForwardDot = FVector::DotProduct(Forward, NormalizedVel);
    float RightDot = FVector::DotProduct(Right, NormalizedVel);

    MoveDirection = FMath::RadiansToDegrees(FMath::Atan2(RightDot, ForwardDot));
    MoveDirection = FMath::Clamp(MoveDirection, -45.f, 45.f);

    /** Shield Block Detect */

    if (CachedCombatComponent)
    {
        int32 CurrentShieldCount = CachedCombatComponent->ShieldBlockCount;

        if (CurrentShieldCount < PreviousShieldCount)
        {
            PlayBlockMontage();
        }

        PreviousShieldCount = CurrentShieldCount;
    }
}

// 공격 애니메이션 재생 함수
void UMyAnimInstance::PlayAttackMontage()
{
    if (!AttackMontage) return;

    if (!Montage_IsPlaying(AttackMontage))
    {
        bIsAttacking = true;
        Montage_Play(AttackMontage);
    }
}

// 방어 애니메이션 재생 함수
void UMyAnimInstance::PlayBlockMontage()
{
    if (!BlockMontage) return;

    if (!Montage_IsPlaying(BlockMontage))
    {
        bIsBlocking = true;
        Montage_Play(BlockMontage);
    }
}

// 상호작용 애니메이션 재생 함수
void UMyAnimInstance::PlayInteractMontage()
{
    if (!InteractMontage) return;

    if (!Montage_IsPlaying(InteractMontage))
    {
        bIsInteracting = true;
        Montage_Play(InteractMontage);
    }
}