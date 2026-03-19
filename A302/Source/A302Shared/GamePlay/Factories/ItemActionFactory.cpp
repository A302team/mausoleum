#include "GamePlay/Factories/ItemActionFactory.h"

#include "GameData/Items/ItemDefinition.h"
#include "GameData/Items/ItemInstance.h"
#include "GamePlay/Items/BaseItem.h"
#include "GamePlay/Items/ItemCursedSword.h"

UBaseItem* UItemActionFactory::CreateLogic(UObject* Outer, UItemInstance* Instance) const
{
    if (!Outer || !Instance || !Instance->Definition)
    {
        return nullptr;
    }

    const UItemDefinition* Def = Instance->Definition.Get();
    if (!Def)
    {
        return nullptr;
    }

    UClass* LogicClass = Def->ResolveRewardLogicClass();
    const bool bIsCursedSwordLogic = LogicClass && LogicClass->IsChildOf(UItemCursedSword::StaticClass());
    if (Def->RewardCategory != ERewardCategory::BasicItem && !bIsCursedSwordLogic)
    {
        return nullptr;
    }

    if (!LogicClass || !LogicClass->IsChildOf(UBaseItem::StaticClass()))
    {
        return nullptr;
    }

    UBaseItem* Logic = NewObject<UBaseItem>(Outer, LogicClass);
    if (!Logic)
    {
        return nullptr;
    }

    Logic->Initialize(Instance);
    return Logic;
}

