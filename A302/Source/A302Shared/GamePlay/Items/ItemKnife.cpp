#include "GamePlay/Items/ItemKnife.h"

#include "Animation/MyAnimInstance.h"
#include "Character/MyCharacter.h"
#include "Engine/World.h"
#include "GameData/Items/ItemDefinition.h"
#include "GameData/Items/ItemInstance.h"
#include "GameFramework/Character.h"
#include "GamePlay/Actor/WeaponActor.h"
#include "Character/MyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "A302GameplayGuards.h"
#include "Character/Components/TraceHelper.h"
#include "Character/Components/Combat/EquipmentComponent.h"

bool UItemKnife::CanUse_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData) const
{
    if (!Instigator)
    {
        return false;
    }

    const UItemDefinition* Def = GetDefinition();
    if (!Def)
    {
        return false;
    }

    AActor* TargetActor = TargetData.TargetActor;
    if (!TargetActor || TargetActor == Instigator)
    {
        return false;
    }

    if (!A302GameplayGuards::CanInstigatorAffectTargetActor(Instigator, TargetActor))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[ItemKnife] CanUse failed: interaction guard blocked target. instigator=%s target=%s"),
            *GetNameSafe(Instigator),
            *GetNameSafe(TargetActor)
        );
        return false;
    }

    const float AllowedRange = FMath::Max(Def->Payload.ItemUseRange, 50.0f);
    const float InstigatorRadius = Instigator->GetSimpleCollisionRadius();

    float TargetRadius = 0.0f;
    if (const ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
    {
        TargetRadius = TargetCharacter->GetSimpleCollisionRadius();
    }

    const float RawDistance = FVector::Dist(Instigator->GetActorLocation(), TargetActor->GetActorLocation());
    const float EdgeDistance = FMath::Max(0.0f, RawDistance - InstigatorRadius - TargetRadius);

    if (EdgeDistance > AllowedRange)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[ItemKnife] CanUse failed: target out of range. edge=%.1f allowed=%.1f raw=%.1f target=%s"),
            EdgeDistance,
            AllowedRange,
            RawDistance,
            *GetNameSafe(TargetActor)
        );
        return false;
    }

    if (Def->Payload.bRequiresLineOfSight && !HasLineOfSight(Instigator, TargetActor))
    {
        UE_LOG(LogTemp, Warning, TEXT("[ItemKnife] CanUse failed: no line of sight to %s"), *GetNameSafe(TargetActor));
        return false;
    }

    return true;
}

bool UItemKnife::Use_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData)
{
    if (!CanUse_Implementation(Instigator, TargetData))
    {
        return false;
    }

    PlayUsePresentation(Instigator);

    AActor* Target = TargetData.TargetActor;
    AController* InstigatorController = Instigator ? Instigator->GetController() : nullptr;

    if (Instigator && Instigator->HasAuthority())
    {
        const float Damage = 50.0f;
        UGameplayStatics::ApplyDamage(
            Target,
            Damage,
            InstigatorController,
            Instigator,
            nullptr
        );
    }

    if (UItemInstance* Inst = GetInstance())
    {
        Inst->Consume(1);
    }

    if (AMyCharacter* MyCharacter = Cast<AMyCharacter>(Instigator))
    {
        OnItemUsed(MyCharacter);
    }

    return true;
}

bool UItemKnife::ResolveServerTargetedUse(ACharacter* OwnerCharacter, AActor* TargetActor, FString& OutSystemMessage) const
{
    OutSystemMessage.Empty();

    if (!OwnerCharacter || !OwnerCharacter->HasAuthority() || !TargetActor || TargetActor == OwnerCharacter)
    {
        return false;
    }

    if (!A302GameplayGuards::CanInstigatorAffectTargetActor(OwnerCharacter, TargetActor))
    {
        return false;
    }

    constexpr float DefaultAllowedRange = 200.0f;
    const float InstigatorRadius = OwnerCharacter->GetSimpleCollisionRadius();
    float TargetRadius = 0.0f;
    if (const ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
    {
        TargetRadius = TargetCharacter->GetSimpleCollisionRadius();
    }

    const float RawDistance = FVector::Dist(OwnerCharacter->GetActorLocation(), TargetActor->GetActorLocation());
    const float EdgeDistance = FMath::Max(0.0f, RawDistance - InstigatorRadius - TargetRadius);
    if (EdgeDistance > DefaultAllowedRange)
    {
        return false;
    }

    UGameplayStatics::ApplyDamage(
        TargetActor,
        50.0f,
        OwnerCharacter->GetController(),
        OwnerCharacter,
        nullptr
    );

    return true;
}

void UItemKnife::PlayUsePresentation(ACharacter* Instigator)
{
    AMyCharacter* MyCharacter = Cast<AMyCharacter>(Instigator);
    if (!MyCharacter)
    {
        return;
    }

    if (UEquipmentComponent* EquipmentComp = MyCharacter->GetEquipmentComponent())
    {
        if (EquipmentComp->KnifeActorClass)
        {
            EquipmentComp->EquipWeapon(EquipmentComp->KnifeActorClass);
        }
    }

    if (UMyAnimInstance* Anim = Cast<UMyAnimInstance>(MyCharacter->GetMesh()->GetAnimInstance()))
    {
        Anim->PlayAttackMontage();
    }
}

bool UItemKnife::HasLineOfSight(ACharacter* Instigator, AActor* Target) const
{
    if (!Instigator || !Target)
    {
        return false;
    }

    UWorld* World = Instigator->GetWorld();
    if (!World)
    {
        return false;
    }

    FVector Start = Instigator->GetActorLocation();
    FVector UnusedDirection = FVector::ForwardVector;
    TraceHelper::TryGetCrosshairTrace(Instigator, Start, UnusedDirection);
    const FVector End = Target->GetActorLocation();

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(KnifeLOS), false, Instigator);

    const bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
    if (!bHit)
    {
        return true;
    }

    return Hit.GetActor() == Target;
}
