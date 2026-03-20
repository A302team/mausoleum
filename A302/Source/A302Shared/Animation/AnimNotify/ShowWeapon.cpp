#include "Animation/AnimNotify/ShowWeapon.h"
#include "GameFramework/Character.h"

void UShowWeapon::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp || !WeaponClass) return;

    ACharacter* OwnerCharacter = Cast<ACharacter>(MeshComp->GetOwner());
    if (!OwnerCharacter) return;

    // 무기 생성
    AWeaponActor* SpawnedWeapon = MeshComp->GetWorld()->SpawnActor<AWeaponActor>(WeaponClass);

    if (!SpawnedWeapon) return;

    // 소켓에 부착
    SpawnedWeapon->AttachToComponent(
        MeshComp,
        FAttachmentTransformRules::SnapToTargetNotIncludingScale,
        SocketName
    );
}