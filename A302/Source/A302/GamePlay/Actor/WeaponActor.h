#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponActor.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;

UCLASS()
class A302_API AWeaponActor : public AActor
{
    GENERATED_BODY()

public:

    AWeaponActor();

    void ShowWeapon();
    void HideWeapon();

    void AttachToCharacter(USkeletalMeshComponent* CharacterMesh, FName SocketName);

protected:

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* WeaponMesh;
};