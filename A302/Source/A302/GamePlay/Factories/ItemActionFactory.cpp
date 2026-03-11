#include "GamePlay/Factories/ItemActionFactory.h"

#include "GameData/ItemDefinition.h"
#include "GameData/ItemInstance.h"
#include "GameData/RewardTypes.h"
#include "GamePlay/Items/BaseItem.h"

UBaseItem* UItemActionFactory::CreateLogic(UObject* Outer, UItemInstance* Instance) const
{
    if (!Outer || !Instance || !Instance->Definition)
    {
        return nullptr;
    }

    const UItemDefinition* Def = Instance->Definition.Get();
    if (!Def || Def->RewardCategory != ERewardCategory::BasicItem)
    {
        return nullptr;
    }

    UClass* LogicClass = Def->ResolveRewardLogicClass();
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
