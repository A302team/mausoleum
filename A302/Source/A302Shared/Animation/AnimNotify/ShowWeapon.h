#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ShowWeapon.generated.h"

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
    Knife,
    CursedSword,
    Shield
};

UCLASS()
class A302SHARED_API UShowWeapon : public UAnimNotify
{
    GENERATED_BODY()

public:

    // 애니메이션에서 무기 선택
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
    EWeaponType WeaponType = EWeaponType::Knife;

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};