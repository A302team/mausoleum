#include "GamePlay/Items/ItemShield.h"
#include "GameData/ItemDefinition.h"
#include "GameData/ItemInstance.h"
#include "GameFramework/Character.h"
#include "Character/Components/CombatStatusComponent.h"

bool UItemShield::CanUse_Implementation(ACharacter* Instigator, const FItemTargetData& /*TargetData*/) const
{
    if (!Instigator) return false;
    const UItemDefinition* Def = GetDefinition();
    if (!Def) return false;

    // 방패는 SelfCast라 타겟 필요 없음
    // 추가 룰: 이미 방패가 있으면 못 쓰게 하고 싶으면 여기서 체크
    return true;
}

bool UItemShield::Use_Implementation(ACharacter* Instigator, const FItemTargetData& TargetData)
{
    if (!CanUse_Implementation(Instigator, TargetData)) return false;

    const UItemDefinition* Def = GetDefinition();
    if (!Def) return false;

    UCombatStatusComponent* Combat = Instigator->FindComponentByClass<UCombatStatusComponent>();
    if (!Combat) return false;

    Combat->AddShield(Def->BlockCount);

    if (UItemInstance* Inst = GetInstance())
    {
        Inst->Consume(1);
    }

    return true;
}
