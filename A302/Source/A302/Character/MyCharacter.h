// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Interface/InteractableInterface.h"
#include "MyCharacter.generated.h"

class UInputAction;
class UUserWidget;
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

    // Called every frame
    virtual void Tick(float DeltaTime) override;

    // Called to bind functionality to input
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // Input Actions
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction* IA_Move;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction* IA_Look;

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction* IA_Jump;
    
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction* IA_Interact;

    // 상호작용 가능 거리를 설정합니다.
    UPROPERTY(EditAnywhere, Category = "Interaction")
    float InteractionDistance = 300.f;

    // 매 프레임 상호작용 대상을 체크하는 함수
    void CheckForInteractables();
    
    void ToggleHighlight(AActor* TargetActor, bool bIsOn);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UUserWidget> InteractionWidgetClass;
    
    UPROPERTY()
    UUserWidget* InteractionWidgetInstance;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    TSubclassOf<UUserWidget> CrosshairWidgetClass;
    
    UPROPERTY()
    UUserWidget* CrosshairWidgetInstance;

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

    void SetupKnifeForTest();
    void FindAndWarpNearDummy();
    void ExecuteAutoKnifeAttack();

private:
    void OnMove(const FInputActionValue& Value);
    void OnLook(const FInputActionValue& Value);
    void OnJump(const FInputActionValue& Value);
    void OnJumpReleased(const FInputActionValue& Value);
    void OnInteract(const FInputActionValue& Value);

    UPROPERTY()
    AActor* LastInteractableActor = nullptr;

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