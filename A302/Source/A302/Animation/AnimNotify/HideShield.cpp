#include "Animation/AnimNotify/HideShield.h"
#include "Character/MyCharacter.h"
#include "GamePlay/Actor/ShieldActor.h"

void UHideShield::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp) return;

    AMyCharacter* Character = Cast<AMyCharacter>(MeshComp->GetOwner());
    if (!Character) return;

    if (Character->CurrentShield)
    {
        Character->CurrentShield->Destroy();
        Character->CurrentShield = nullptr;
    }
}