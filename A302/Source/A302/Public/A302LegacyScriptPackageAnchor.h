#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "A302LegacyScriptPackageAnchor.generated.h"

/**
 * Keeps /Script/A302 alive for legacy Blueprint references during module split migration.
 */
UCLASS()
class A302_API UA302LegacyScriptPackageAnchor : public UObject
{
    GENERATED_BODY()
};
