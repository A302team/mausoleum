#include "GamePlay/Actor/WeaponActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"

AWeaponActor::AWeaponActor()
{
    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    RootComponent = WeaponMesh;

    // 무기 충돌 제거 (캐릭터 밀림 방지)
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponMesh->SetGenerateOverlapEvents(false);
}

void AWeaponActor::ShowWeapon()
{
    if (WeaponMesh)
    {
        WeaponMesh->SetVisibility(true);
    }
}

void AWeaponActor::HideWeapon()
{
    if (WeaponMesh)
    {
        WeaponMesh->SetVisibility(false);
    }
}

void AWeaponActor::AttachToCharacter(USkeletalMeshComponent* CharacterMesh, FName SocketName)
{
    if (!CharacterMesh) return;

    AttachToComponent(
        CharacterMesh,
        FAttachmentTransformRules::SnapToTargetNotIncludingScale,
        SocketName
    );
}