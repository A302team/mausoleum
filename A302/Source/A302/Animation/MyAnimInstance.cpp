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

// 애니메이션 업데이트 함수 (매 프레임 호출)
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

    // Detect Shield Use → Play Block Animation
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

// 블록 몽타주 재생 함수 (방패 사용 시 호출)
void UMyAnimInstance::PlayBlockMontage()
{
    if (!BlockMontage)
    {
        return;
    }

    if (!Montage_IsPlaying(BlockMontage))
    {
        Montage_Play(BlockMontage);
    }
}