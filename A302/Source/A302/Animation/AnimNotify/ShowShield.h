#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "ShowShield.generated.h"

UCLASS()
class A302_API UShowShield : public UAnimNotify
{
    GENERATED_BODY()

public:

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

public:

    // 스폰할 ShieldActor 클래스 (BP_ShieldActor 지정)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shield")
    TSubclassOf<class AShieldActor> ShieldClass;
};