#include "Animation/AnimNotify/ShowWeapon.h"
#include "Character/MyCharacter.h"
#include "Character/Components/Combat/EquipmentComponent.h"

void UShowWeapon::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp) return;

    if (AMyCharacter* Character = Cast<AMyCharacter>(MeshComp->GetOwner()))
    {
        if (UEquipmentComponent* Equipment = Character->GetEquipmentComponent())
        {
            switch (WeaponType)
            {
                case EWeaponType::Knife:
                    Equipment->EquipKnifeWeapon();
                    break;

                case EWeaponType::CursedSword:
                    Equipment->EquipCursedSwordWeapon();
                    break;

                case EWeaponType::Shield:
                    Equipment->EquipShieldWeapon();
                    break;
            }
        }
    }
}