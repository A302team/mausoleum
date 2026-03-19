#include "Animation/AnimNotify/HideWeapon.h"
#include "GameFramework/Character.h"
#include "GamePlay/Actor/WeaponActor.h"

void UHideWeapon::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp) return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(MeshComp->GetOwner());
    if (!OwnerCharacter) return;

    // 붙어있는 무기 찾기
    TArray<AActor*> AttachedActors;
    OwnerCharacter->GetAttachedActors(AttachedActors);

    for (AActor* Actor : AttachedActors)
    {
        if (AWeaponActor* Weapon = Cast<AWeaponActor>(Actor))
        {
            Weapon->Destroy();
        }
    }
}