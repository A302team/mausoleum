#include "Animation/MyAnimInstance.h"
#include "Character/Components/Combat/CombatStatusComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

void UMyAnimInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UMyAnimInstance, bIsAttacking);
    DOREPLIFETIME(UMyAnimInstance, bIsBlocking);
    DOREPLIFETIME(UMyAnimInstance, bIsInteracting);
    DOREPLIFETIME(UMyAnimInstance, bIsDead);
}

void UMyAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    CachedCharacter = Cast<ACharacter>(TryGetPawnOwner());

    // 캐릭터와 CombatStatusComponent를 캐싱하여 방어 애니메이션 재생 시 매번 컴포넌트를 찾는 비용을 줄임
    if (CachedCombatComponent)
    {
        int32 CurrentShieldCount = CachedCombatComponent->ShieldBlockCount;

        if (bAnimInitialized && CurrentShieldCount < PreviousShieldCount)
        {
            PlayBlockMontage();
        }

        PreviousShieldCount = CurrentShieldCount;
    }

    bAnimInitialized = true;
}

void UMyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    if (!CachedCharacter)
    {
        CachedCharacter = Cast<ACharacter>(TryGetPawnOwner());
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

    if (!CachedCombatComponent && CachedCharacter)
    {
        CachedCombatComponent = CachedCharacter->FindComponentByClass<UCombatStatusComponent>();
    }

    if (CachedCombatComponent)
    {
        int32 CurrentShieldCount = CachedCombatComponent->ShieldBlockCount;

        if (CurrentShieldCount < PreviousShieldCount && CachedCharacter->HasAuthority())
        {
            PlayBlockMontage();
        }

        PreviousShieldCount = CurrentShieldCount;
    }
}


// 공격 애니메이션 재생
void UMyAnimInstance::PlayAttackMontage()
{
    if (!AttackMontage) return;

    if (!Montage_IsPlaying(AttackMontage))
    {
        bIsAttacking = true;
        Montage_Play(AttackMontage);
    }
}

void UMyAnimInstance::PlayAttackAnimation()
{
    PlayAttackMontage();
}


// 방어 애니메이션 재생
void UMyAnimInstance::PlayBlockMontage()
{   
    if (!BlockMontage) return;

    if (!Montage_IsPlaying(BlockMontage))
    {
        bIsBlocking = true;
        Montage_Play(BlockMontage);
    }
}

void UMyAnimInstance::PlayBlockAnimation()
{
    PlayBlockMontage();
}


// 상호작용 애니메이션 재생
void UMyAnimInstance::PlayInteractMontage()
{
    if (!InteractMontage) return;

    if (!Montage_IsPlaying(InteractMontage))
    {
        bIsInteracting = true;
        Montage_Play(InteractMontage);
    }
}

void UMyAnimInstance::PlayInteractAnimation()
{
    PlayInteractMontage();
}

// 사망 애니메이션 재생
void UMyAnimInstance::PlayDeathMontage()
{
    if (!DeathMontage) return;

    if (!Montage_IsPlaying(DeathMontage))
    {
        bIsDead = true;
        Montage_Play(DeathMontage);
    }
}

void UMyAnimInstance::PlayDeathAnimation()
{
    PlayDeathMontage();
}

// 타임 나이프(피바라기) 애니메이션 재생
void UMyAnimInstance::PlayTimeKnifeMontage()
{
    if (!TimeKnifeMontage) return;

    if (!Montage_IsPlaying(TimeKnifeMontage))
    {
        bIsAttacking = true;
        Montage_Play(TimeKnifeMontage);
    }
}

void UMyAnimInstance::PlayTimeKnifeAnimation()
{
    PlayTimeKnifeMontage();
}

// 석상 상호작용 애니메이션 재생
void UMyAnimInstance::PlayStatueInteractMontage()
{
    if (!StatueInteractMontage) return;

    if (!Montage_IsPlaying(StatueInteractMontage))
    {
        Montage_Play(StatueInteractMontage);
    }
}

void UMyAnimInstance::StopStatueInteractMontage()
{
    if (!StatueInteractMontage) return;

    if (Montage_IsPlaying(StatueInteractMontage))
    {
        Montage_Stop(0.25f, StatueInteractMontage);
    }
}

void UMyAnimInstance::PlayStatueInteractAnimation()
{
    PlayStatueInteractMontage();
}

void UMyAnimInstance::StopStatueInteractAnimation()
{
    StopStatueInteractMontage();
}
