#include "GamePlay/Items/BaseItem.h"
#include "GameData/Items/ItemInstance.h"
#include "GameData/Items/ItemDefinition.h"

const UItemDefinition* UBaseItem::GetDefinition() const
{
    return Instance ? Instance->Definition.Get() : nullptr;
}

