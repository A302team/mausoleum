#pragma once

#include "CoreMinimal.h"
#include "ItemTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EItemUseMode : uint8
{
    Targeted UMETA(DisplayName = "Targeted"),
    SelfCast UMETA(DisplayName = "SelfCast"),
    SelfOrTargeted UMETA(DisplayName = "SelfOrTargeted"),
};

USTRUCT(BlueprintType)
struct FItemTargetData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TObjectPtr<AActor> TargetActor = nullptr;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FVector TargetLocation = FVector::ZeroVector;

    bool HasTargetActor() const { return TargetActor != nullptr; }
};
