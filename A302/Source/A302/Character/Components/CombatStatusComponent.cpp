#include "Character/Components/CombatStatusComponent.h"

UCombatStatusComponent::UCombatStatusComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCombatStatusComponent::AddShield(int32 Count)
{
    ShieldBlockCount = FMath::Max(0, ShieldBlockCount + Count);
    OnShieldChanged.Broadcast(ShieldBlockCount);
}

bool UCombatStatusComponent::TryConsumeShieldToBlock()
{
    if (ShieldBlockCount <= 0) return false;

    ShieldBlockCount -= 1;
    OnShieldChanged.Broadcast(ShieldBlockCount);
    return true;
}