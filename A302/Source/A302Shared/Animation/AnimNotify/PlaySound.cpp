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

    if (Sound->IsLooping())
    {
        // Guard against log spam when the same invalid notify is triggered repeatedly.
        static TSet<FString> ReportedLoopingNotifyPairs;
        const FString NotifyKey = FString::Printf(TEXT("%s|%s"), *GetNameSafe(Animation), *GetNameSafe(Sound));
        if (!ReportedLoopingNotifyPairs.Contains(NotifyKey))
        {
            ReportedLoopingNotifyPairs.Add(NotifyKey);
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("[PlaySoundNotify] Looping sound '%s' in animation '%s' is not valid for one-shot notify playback. Use AudioComponent or AmbientSound Actor."),
                *GetNameSafe(Sound),
                *GetNameSafe(Animation)
            );
        }
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
