#include "Character/Components/Combat/CombatStatusComponent.h"

UCombatStatusComponent::UCombatStatusComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UCombatStatusComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UCombatStatusComponent, ShieldBlockCount);
}

void UCombatStatusComponent::OnRep_ShieldBlockCount()
{
    OnShieldChanged.Broadcast(ShieldBlockCount);
}

void UCombatStatusComponent::AddShield(int32 Count)
{
    if (Count <= 0 || !GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    ShieldBlockCount = FMath::Max(0, ShieldBlockCount + Count);
    OnShieldChanged.Broadcast(ShieldBlockCount);
}

bool UCombatStatusComponent::TryConsumeShieldToBlock()
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return false;
    }

    if (ShieldBlockCount <= 0) return false;

    ShieldBlockCount -= 1;
    OnShieldChanged.Broadcast(ShieldBlockCount);
    return true;
}
