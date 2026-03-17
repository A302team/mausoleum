#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "SpawnVFX.generated.h"

class UNiagaraSystem;

UCLASS()
class A302CLIENT_API USpawnVFX : public UAnimNotify
{
    GENERATED_BODY()

public:

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

    // 사용할 Niagara VFX
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VFX")
    UNiagaraSystem* NiagaraEffect; // NiagaraEffect == 사용할 VFX

    // 붙일 소켓 이름
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="VFX")
    FName SocketName = "weapon_socket"; // SocketName == VFX가 붙을 소켓
};
