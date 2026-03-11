#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KnifeActor.generated.h"

UCLASS()
class A302_API AKnifeActor : public AActor
{
    GENERATED_BODY()

public:
    AKnifeActor();

protected:

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* KnifeMesh;

public:

    void AttachToCharacter(USkeletalMeshComponent* CharacterMesh, FName SocketName);

    void ShowWeapon();
    void HideWeapon();
};