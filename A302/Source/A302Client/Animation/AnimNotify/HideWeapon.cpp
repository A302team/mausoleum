#include "Animation/AnimNotify/HideWeapon.h"
#include "Interface/A302CharacterBridge.h"

void UHideWeapon::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (IA302CharacterBridge* CharacterBridge = Cast<IA302CharacterBridge>(MeshComp->GetOwner()))
    {
        CharacterBridge->HideWeapon();
    }
}
