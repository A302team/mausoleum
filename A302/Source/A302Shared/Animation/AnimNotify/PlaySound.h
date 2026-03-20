#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "PlaySound.generated.h"

class USoundBase;

UCLASS()
class A302SHARED_API UPlaySound : public UAnimNotify
{
    GENERATED_BODY()

public:

    // 재생할 사운드
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sound")
    USoundBase* Sound;

    // 소켓 이름 (선택)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sound")
    FName SocketName;

    // 볼륨
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sound")
    float VolumeMultiplier = 1.0f;

    // 피치
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sound")
    float PitchMultiplier = 1.0f;

public:

    virtual void Notify(
        USkeletalMeshComponent* MeshComp,
        UAnimSequenceBase* Animation
    ) override;
};
