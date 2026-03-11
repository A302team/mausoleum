#pragma once

#include "CoreMinimal.h"
#include "RewardTypes.generated.h"

UENUM(BlueprintType)
enum class ERewardCategory : uint8
{
    BasicItem      UMETA(DisplayName="Basic Item"),
    PersonalEvent  UMETA(DisplayName="Personal Event"),
    GroupEvent     UMETA(DisplayName="Group Event"),
};
