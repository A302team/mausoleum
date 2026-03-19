#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GamePlay/Actor/WeaponActor.h"
#include "ShowWeapon.generated.h"

UCLASS()
class A302SHARED_API UShowWeapon : public UAnimNotify
{
    GENERATED_BODY()

public:

    // 스폰할 무기
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
    TSubclassOf<AWeaponActor> WeaponClass;

    // 붙일 소켓
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
    FName SocketName = "weapon_socket";

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};