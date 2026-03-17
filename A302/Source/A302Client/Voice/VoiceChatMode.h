#pragma once

#include "VoiceChatMode.generated.h"

UENUM(BlueprintType)
enum class EVoiceChatMode : uint8
{
    Lobby UMETA(DisplayName = "Lobby"),
    InGameDistance UMETA(DisplayName = "InGameDistance")
};
