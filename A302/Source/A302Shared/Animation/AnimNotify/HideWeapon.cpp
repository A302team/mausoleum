#include "Animation/AnimNotify/HideWeapon.h"
#include "Character/MyCharacter.h"
#include "Character/Components/Combat/EquipmentComponent.h"

void UHideWeapon::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp) return;

    if (AMyCharacter* Character = Cast<AMyCharacter>(MeshComp->GetOwner()))
    {
        if (UEquipmentComponent* Equipment = Character->GetEquipmentComponent())
        {
            Equipment->HideWeapon();
        }
    }
}