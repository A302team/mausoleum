// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Interface/InteractableInterface.h"
#include "MyCharacter.generated.h"

class UInputAction;
class UUserWidget;

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
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	UInputAction* IA_Interact;

private:
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnJump(const FInputActionValue& Value);
	void OnJumpReleased(const FInputActionValue& Value);
	void OnInteract(const FInputActionValue& Value);
	
	UPROPERTY()
	AActor* LastInteractableActor = nullptr;
	void ToggleHighlight(AActor* TargetActor, bool bIsOn);
	
protected:
	// 상호작용 가능 거리를 설정합니다.
	UPROPERTY(EditAnywhere, Category = "Interaction")
	float InteractionDistance = 300.f;

	// 매 프레임 상호작용 대상을 체크하는 함수
	void CheckForInteractables();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> InteractionWidgetClass;
	
	UPROPERTY()
	UUserWidget* InteractionWidgetInstance;
};
