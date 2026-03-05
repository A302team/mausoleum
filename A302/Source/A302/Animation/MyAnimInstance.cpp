#include "Animation/MyAnimInstance.h"
#include "Character/MyCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UMyAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    CachedCharacter = Cast<AMyCharacter>(TryGetPawnOwner());
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

    FRotator BaseRotation = CachedCharacter->GetActorRotation();


    FVector Forward = CachedCharacter->GetActorForwardVector();
    FVector Right = CachedCharacter->GetActorRightVector();

    FVector NormalizedVel = Velocity.GetSafeNormal();

    float ForwardDot = FVector::DotProduct(Forward, NormalizedVel);
    float RightDot = FVector::DotProduct(Right, NormalizedVel);

    MoveDirection = FMath::RadiansToDegrees(FMath::Atan2(RightDot, ForwardDot));
    MoveDirection = FMath::Clamp(MoveDirection, -45.f, 45.f);

    //bIsAttacking = CachedCharacter->bIsAttacking;
}