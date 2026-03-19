#include "SpawnVFX.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/SkeletalMeshComponent.h"

void USpawnVFX::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
    if (!MeshComp || !NiagaraEffect)
    {
        return;
    }

    UNiagaraFunctionLibrary::SpawnSystemAttached(
        NiagaraEffect,
        MeshComp,
        SocketName,
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        EAttachLocation::SnapToTarget,
        true
    );
}
