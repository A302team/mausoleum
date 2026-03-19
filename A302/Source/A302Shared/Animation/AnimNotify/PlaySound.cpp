#include "Animation/AnimNotify/PlaySound.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SkeletalMeshComponent.h"
#include "Sound/SoundBase.h"

void UPlaySound::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp || !Sound)
    {
        return;
    }

    FVector Location = MeshComp->GetComponentLocation();

    // 소켓이 지정되어 있으면 소켓 위치 사용
    if (SocketName != NAME_None)
    {
        Location = MeshComp->GetSocketLocation(SocketName);
    }

    UGameplayStatics::PlaySoundAtLocation(
        MeshComp,
        Sound,
        Location,
        VolumeMultiplier,
        PitchMultiplier
    );
}
