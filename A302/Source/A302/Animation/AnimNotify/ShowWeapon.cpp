#include "Animation/AnimNotify/ShowWeapon.h"
#include "Character/MyCharacter.h"

void UShowWeapon::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (AMyCharacter* Character = Cast<AMyCharacter>(MeshComp->GetOwner()))
    {
        Character->ShowWeapon();
    }
}