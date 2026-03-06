// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "TimerManager.h"
#include "Interface/InteractableInterface.h"
#include "MyCharacter.generated.h"

class UInputAction;
class UCombatStatusComponent;
class UItemDefinition;
class UUserWidget;
class UItemActionFactory;
class UItemInstance;
class UBaseItem;
class ADummyCharacter;
class UInteractComponent;
class UMaliceComponent;
class UQuickSlotComponent;
class UKnifeAutoTestComponent;
class UPrivateVoiceChatComponent;

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

    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction* IA_VoiceChat;

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
    
    // UI에서 가져다 쓸 진행도 비율 (0.0 ~ 1.0)
    UPROPERTY(BlueprintReadOnly, Category = "Interaction")
    float InteractProgressRatio = 0.0f;
    
    UPROPERTY(EditAnywhere, Category = "Interaction")
    float MaxInteractHoldTime = 2.0f;

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

    // Interact
    void OnInteractComplete(const FInputActionValue& Value);
	void OnItemSelect(const FInputActionValue& Value);
	void OnAttack(const FInputActionValue& Value);
	
	void OnInteractHoldComplete(const FInputActionValue& Value);
	void OnInteractHoldProgress(const FInputActionValue& Value);
	void OnInteractHoldCanceled(const FInputActionValue& Value);
	
	void OnQTEInteractStarted(const FInputActionValue& Value);
    void OnInteractProgress(const FInputActionValue& Value);
    void OnInteractCanceled(const FInputActionValue& Value);
    void OnToggleVoiceChat(const FInputActionValue& Value);

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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voice", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UPrivateVoiceChatComponent> PrivateVoiceChatComponent = nullptr;
    
    int32 AutoAttackCount = 0;
};
