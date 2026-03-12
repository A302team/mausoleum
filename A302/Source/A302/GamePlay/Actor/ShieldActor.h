#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShieldActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class A302_API AShieldActor : public AActor
{
    GENERATED_BODY()

public:

    AShieldActor();

    void AttachToCharacter(USkeletalMeshComponent* CharacterMesh, FName SocketName);

protected:

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* ShieldMesh;
};