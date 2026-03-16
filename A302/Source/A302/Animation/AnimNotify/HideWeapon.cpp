#include "Animation/AnimNotify/HideWeapon.h"
#include "Character/MyCharacter.h"

void UHideWeapon::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (AMyCharacter* Character = Cast<AMyCharacter>(MeshComp->GetOwner()))
    {
        Character->HideWeapon();
    }
}