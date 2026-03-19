#include "Animation/AnimNotify/HideWeapon.h"
#include "Character/MyCharacter.h"
#include "Character/Components/Combat/EquipmentComponent.h"

void UHideWeapon::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (AMyCharacter* CharacterBridge = Cast<AMyCharacter>(MeshComp->GetOwner()))
    {
        if (UEquipmentComponent* EquipmentComp = CharacterBridge->GetEquipmentComponent())
        {
            EquipmentComp->HideWeapon();
        }
    }
}
