#include "Character/Items/ItemKnife.h"
#include "GameData/ItemDefinition.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

bool UItemKnife::CanUse_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData) const
{
    if (!Instigator) return false;

    const UItemDefinition* Def = GetDefinition();
    if (!Def) return false;

    if (!TargetData.TargetActor) return false;

    const FVector A = Instigator->GetActorLocation();
    const FVector B = TargetData.TargetActor->GetActorLocation();
    const float Dist = FVector::Dist(A, B);

    if (Dist > Def->AttackRange) return false;

    if (Def->bRequiresLineOfSight)
    {
        if (!HasLineOfSight(Instigator, TargetData.TargetActor))
            return false;
    }

    return true;
}

bool UItemKnife::Use_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData)
{
    if (!CanUse_Implementation(Instigator, TargetData)) return false;

    // ✅ 데미지 적용: ApplyDamage는 언리얼 표준 “피해 전달 루트”
    // - 실제 체력 감소는 TargetActor의 TakeDamage / Damage Interface / HealthComponent에서 처리
    AActor* Target = TargetData.TargetActor;
    AController* InstigatorController = Instigator ? Instigator->GetController() : nullptr;

    const float Damage = 50.0f; // MVP 고정값(나중에 Def로 이동)
    UGameplayStatics::ApplyDamage(
        Target,
        Damage,
        InstigatorController,
        Instigator,
        nullptr // DamageTypeClass (필요하면 지정)
    );

    // 소모 아이템이면 1개 소모
    if (UItemInstance* Inst = GetInstance())
    {
        Inst->Consume(1);
    }

    return true;
}

bool UItemKnife::HasLineOfSight(ACharacter* Instigator, AActor* Target) const
{
    if (!Instigator || !Target) return false;
    UWorld* World = Instigator->GetWorld();
    if (!World) return false;

    const FVector Start = Instigator->GetActorLocation();
    const FVector End   = Target->GetActorLocation();

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(KnifeLOS), false, Instigator);
    Params.AddIgnoredActor(Target); // “타겟이 아니라 장애물만” 보고 싶으면 이건 빼도 됨

    const bool bHit = World->LineTraceSingleByChannel(
        Hit,
        Start,
        End,
        ECC_Visibility,
        Params
    );

    // bHit=false면 시야 막힘 없음.
    // bHit=true면 뭔가 맞았다는 의미인데, 여기선 간단히 "막힘"으로 취급.
    return !bHit;
}