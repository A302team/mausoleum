#include "Animation/AnimNotify/ShowWeapon.h"
#include "Character/MyCharacter.h"
#include "Character/Components/Combat/EquipmentComponent.h"

void UShowWeapon::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (AMyCharacter* CharacterBridge = Cast<AMyCharacter>(MeshComp->GetOwner()))
    {
        if (UEquipmentComponent* EquipmentComp = CharacterBridge->GetEquipmentComponent())
        {
            EquipmentComp->ShowWeapon();
        }
    }
}
