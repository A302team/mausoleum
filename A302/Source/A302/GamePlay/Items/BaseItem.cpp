#include "GamePlay/Items/BaseItem.h"
#include "GameData/ItemInstance.h"
#include "GameData/ItemDefinition.h"

const UItemDefinition* UBaseItem::GetDefinition() const
{
    return Instance ? Instance->Definition.Get() : nullptr;
}
