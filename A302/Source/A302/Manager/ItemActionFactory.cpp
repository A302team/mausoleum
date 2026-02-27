#include "Manager/ItemActionFactory.h"
#include "GameData/ItemInstance.h"
#include "GameData/ItemDefinition.h"
#include "Character/Items/BaseItem.h"

UBaseItem* UItemActionFactory::CreateLogic(UObject* Outer, UItemInstance* Instance) const
{
    if (!Outer || !Instance || !Instance->Definition)
    {
        return nullptr;
    }

    const UItemDefinition* Def = Instance->Definition.Get();
    if (!Def->ItemLogicClass)
    {
        return nullptr;
    }

    // ✅ UE 핵심: NewObject로 UObject 인스턴스 생성
    UBaseItem* Logic = NewObject<UBaseItem>(Outer, Def->ItemLogicClass);
    if (!Logic) return nullptr;

    Logic->Initialize(Instance);
    return Logic;
}