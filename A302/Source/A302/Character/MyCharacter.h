// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "TimerManager.h"
#include "MyCharacter.generated.h"

class UInputAction;
class UCombatStatusComponent;
class UItemDefinition;
class UKnifeAutoTestComponent;
class UInteractComponent;
class UMaliceComponent;
class UQuickSlotComponent;

UCLASS()
class A302_API AMyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMyCharacter();
	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser
	) override;
	void NotifyKilledCharacter();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;

	// BP can bind animation/FX here after a successful item use.
	UFUNCTION(BlueprintImplementableEvent, Category = "Item|Action")
	void BP_OnPrimaryItemUsed(UItemDefinition* UsedItemDefinition, int32 UsedSlotNumberOneBased);

private:
	UFUNCTION()
	void HandleShieldChanged(int32 NewCount);

	UFUNCTION()
	void HandleMaliceChanged(int32 NewCount);

	void CompleteTimedKnifeObjective();
	void StartTimedKnifeCountdown(const UItemDefinition* TimedKnifeDefinition);
	void TickTimedKnifeCountdown();
	void ClearTimedKnifeState(bool bHideTimer);
	void HandleDead();

	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnJump(const FInputActionValue& Value);
	void OnJumpReleased(const FInputActionValue& Value);

	void OnInteract(const FInputActionValue& Value);
	void OnItemSelect(const FInputActionValue& Value);
	void OnAttack(const FInputActionValue& Value);
	
	void OnInteractHoldComplete(const FInputActionValue& Value);
	void OnInteractHoldProgress(const FInputActionValue& Value);
	void OnInteractHoldCanceled(const FInputActionValue& Value);
	
	void OnQTEInteractStarted(const FInputActionValue& Value);

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Move = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Look = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Jump = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Interact = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_ItemSelect = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Attack = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInteractComponent> InteractionComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|QuickSlot", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UQuickSlotComponent> QuickSlotComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatStatusComponent> CombatStatusComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Malice", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaliceComponent> MaliceComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Test", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UKnifeAutoTestComponent> KnifeAutoTestComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ending", meta = (AllowPrivateAccess = "true"))
	bool bMaliceEnding = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ending", meta = (AllowPrivateAccess = "true"))
	bool bNiceEnding = true;

	bool bHasActiveTimedKnife = false;
	float TimedKnifeRemainingSeconds = 0.0f;
	FName ActiveTimedKnifeItemId = NAME_None;
	FTimerHandle TimedKnifeTimerHandle;

	bool bIsDead = false;
};
