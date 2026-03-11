#include "GamePlay/Actor/KnifeActor.h"
#include "Components/StaticMeshComponent.h"

AKnifeActor::AKnifeActor()
{
    PrimaryActorTick.bCanEverTick = false;

    KnifeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KnifeMesh"));
    RootComponent = KnifeMesh;

    KnifeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    SetActorHiddenInGame(true); // 기본 숨김
}

void AKnifeActor::AttachToCharacter(USkeletalMeshComponent* CharacterMesh, FName SocketName)
{
    AttachToComponent(
        CharacterMesh,
        FAttachmentTransformRules::SnapToTargetNotIncludingScale,
        SocketName
    );
}

void AKnifeActor::ShowWeapon()
{
    SetActorHiddenInGame(false);
}

void AKnifeActor::HideWeapon()
{
    SetActorHiddenInGame(true);
}