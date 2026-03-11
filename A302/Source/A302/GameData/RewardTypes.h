#pragma once

#include "CoreMinimal.h"
#include "RewardTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EItemUseMode : uint8
{
    Targeted UMETA(DisplayName="Targeted"),
    SelfCast UMETA(DisplayName="SelfCast"),
};

UENUM(BlueprintType)
enum class ERewardCategory : uint8
{
    BasicItem      UMETA(DisplayName="Basic Item"),
    PersonalEvent  UMETA(DisplayName="Personal Event"),
    GroupEvent     UMETA(DisplayName="Group Event"),
};

USTRUCT(BlueprintType)
struct FItemTargetData
{
    GENERATED_BODY()

    // 칼은 TargetActor를 사용, 방패는 무시(SelfCast)
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TObjectPtr<AActor> TargetActor = nullptr;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FVector TargetLocation = FVector::ZeroVector;

    bool HasTargetActor() const { return TargetActor != nullptr; }
};
