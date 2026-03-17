#include "Animation/AnimNotify/ShowWeapon.h"
#include "Interface/A302CharacterBridge.h"

void UShowWeapon::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (IA302CharacterBridge* CharacterBridge = Cast<IA302CharacterBridge>(MeshComp->GetOwner()))
    {
        CharacterBridge->ShowWeapon();
    }
}
