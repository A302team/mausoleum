#include "GamePlay/Items/ItemShield.h"
#include "Character/Components/Combat/CombatStatusComponent.h"
#include "GameData/Items/ItemInstance.h"
#include "GameFramework/Character.h"
#include "Character/MyCharacter.h"
#include "Animation/MyAnimInstance.h"
#include "Character/Components/Combat/EquipmentComponent.h"

bool UItemShield::CanUse_Implementation(ACharacter* Instigator, const FItemTargetData& /*TargetData*/) const
{
    if (!Instigator)
    {
        return false;
    }

    if (!Instigator->HasAuthority())
    {
        return false;
    }

    if (!GetDefinition())
    {
        return false;
    }

    UCombatStatusComponent* Combat = Instigator->FindComponentByClass<UCombatStatusComponent>();
    if (!Combat)
    {
        return false;
    }

    return Combat->ShieldBlockCount > 0;
}

bool UItemShield::Use_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData)
{
    if (!CanUse_Implementation(Instigator, TargetData))
    {
        return false;
    }

    if (!GetDefinition())
    {
        return false;
    }

    UCombatStatusComponent* Combat = Instigator->FindComponentByClass<UCombatStatusComponent>();
    if (!Combat)
    {
        return false;
    }

    if (!Combat->TryConsumeShieldToBlock())
    {
        return false;
    }

    if (UItemInstance* Inst = GetInstance())
    {
        Inst->Consume(1);
    }

    if (AMyCharacter* MyCharacter = Cast<AMyCharacter>(Instigator))
    {
        OnItemUsed(MyCharacter);

        if (UEquipmentComponent* EquipmentComp = MyCharacter->GetEquipmentComponent())
        {
            EquipmentComp->EquipWeapon(EquipmentComp->ShieldActorClass);
        }

        if (UMyAnimInstance* Anim = Cast<UMyAnimInstance>(MyCharacter->GetMesh()->GetAnimInstance()))
        {
            Anim->PlayBlockMontage();
        }
    }

    return true;
}
