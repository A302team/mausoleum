#include "GamePlay/Actor/ShieldActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"

AShieldActor::AShieldActor()
{
    ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldMesh"));
    RootComponent = ShieldMesh;

    // 캐릭터 밀림 방지
    ShieldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ShieldMesh->SetGenerateOverlapEvents(false);
}

void AShieldActor::AttachToCharacter(USkeletalMeshComponent* CharacterMesh, FName SocketName)
{
    if (!CharacterMesh) return;

    AttachToComponent(
        CharacterMesh,
        FAttachmentTransformRules::SnapToTargetNotIncludingScale,
        SocketName
    );
}