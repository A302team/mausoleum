#include "Animation/AnimNotify/ShowShield.h"
#include "Character/MyCharacter.h"
#include "GamePlay/Actor/ShieldActor.h"

void UShowShield::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp) return;

    AMyCharacter* Character = Cast<AMyCharacter>(MeshComp->GetOwner());
    if (!Character) return;

    if (!ShieldClass) return;

    UWorld* World = Character->GetWorld();
    if (!World) return;

    // 이미 Shield가 있다면 제거 (중복 생성 방지)
    if (Character->CurrentShield)
    {
        Character->CurrentShield->Destroy();
        Character->CurrentShield = nullptr;
    }

    AShieldActor* Shield = World->SpawnActor<AShieldActor>(ShieldClass);

    if (Shield)
    {
        Shield->AttachToCharacter(MeshComp, TEXT("HandGrip_L"));
        Character->CurrentShield = Shield;
    }
}