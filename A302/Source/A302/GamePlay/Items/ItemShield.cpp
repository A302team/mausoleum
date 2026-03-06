#include "GamePlay/Items/ItemShield.h"
#include "Character/Components/CombatStatusComponent.h"
#include "GameData/ItemInstance.h"
#include "GameFramework/Character.h"

bool UItemShield::CanUse_Implementation(ACharacter* Instigator, const FItemTargetData& /*TargetData*/) const
{
    if (!Instigator)
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

    return true;
}
