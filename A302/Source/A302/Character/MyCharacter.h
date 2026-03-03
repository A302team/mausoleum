// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "MyCharacter.generated.h"

class UInputAction;
class UItemDefinition;
class UItemActionFactory;
class UItemInstance;
class UBaseItem;
class ADummyCharacter;

UCLASS()
class A302_API AMyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMyCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Move;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Look;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Jump;

    UPROPERTY(EditDefaultsOnly, Category = "Item|Definition")
    TObjectPtr<UItemDefinition> KnifeDef;

    UPROPERTY(EditDefaultsOnly, Category = "Item|Definition")
    TObjectPtr<UItemDefinition> ShieldDef;

    UPROPERTY(EditAnywhere, Category = "Item|Test", meta=(ClampMin="0"))
    int32 InitialKnifeStack = 2;

    UPROPERTY(EditAnywhere, Category = "Item|Test", meta=(ClampMin="0.0"))
    float FirstAutoAttackDelay = 0.5f;

    UPROPERTY(EditAnywhere, Category = "Item|Test", meta=(ClampMin="0.0"))
    float SecondAutoAttackDelay = 1.5f;

    UPROPERTY(EditAnywhere, Category = "Item|Test", meta=(ClampMin="0.0"))
    float AutoWarpDistanceToDummy = 120.f;

private:
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnJump(const FInputActionValue& Value);
	void OnJumpReleased(const FInputActionValue& Value);

    void SetupKnifeForTest();
    void FindAndWarpNearDummy();
    void ExecuteAutoKnifeAttack();

    UPROPERTY()
    TObjectPtr<UItemActionFactory> ItemActionFactory;

    UPROPERTY()
    TObjectPtr<UItemInstance> KnifeInstance;

    UPROPERTY()
    TObjectPtr<UBaseItem> KnifeLogic;

    UPROPERTY()
    TObjectPtr<ADummyCharacter> CachedDummyCharacter;

    int32 AutoAttackCount = 0;
};
