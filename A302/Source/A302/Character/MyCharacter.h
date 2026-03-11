// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/StaticMeshComponent.h"
#include "GamePlay/Actor/KnifeActor.h"
#include "InputActionValue.h"
#include "TimerManager.h"
#include "Interface/InteractableInterface.h"
#include "MyCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UCombatStatusComponent;
class UItemDefinition;
class UUserWidget;
class UItemActionFactory;
class UItemInstance;
class UBaseItem;
class ADummyCharacter;
class UKnifeAutoTestComponent;
class UInteractComponent;
class UMaliceComponent;
class UQuickSlotComponent;
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
    
    void SetQTEInputMode(bool bIsQTE);

		// 무기 표시
		void ShowKnife();
    void HideKnife();

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    // Input Actions
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> IMC_Default = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> IMC_QTE = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Move = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Look = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Jump = nullptr;
    
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Interact = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_ESC = nullptr;

    // 상호작용 가능 거리를 설정합니다.
    UPROPERTY(EditAnywhere, Category = "Interaction")
    float InteractionDistance = 300.f;

    // 매 프레임 상호작용 대상을 체크하는 함수
    void CheckForInteractables();
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_VoiceChat = nullptr;
    
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_ItemSelect = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Attack = nullptr;
    
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_QTE_Input = nullptr;
	
	// Item
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

	UFUNCTION(BlueprintImplementableEvent, Category = "Item|Action")
	void BP_OnPrimaryItemUsed(UItemDefinition* UsedItemDefinition, int32 UsedSlotNumberOneBased);

    virtual void GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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
	void InteractionCompleteResult();
	void OnItemSelect(const FInputActionValue& Value);
	void OnAttack(const FInputActionValue& Value);
    
	void OnInteractHoldProgress(const FInputActionValue& Value);
	void OnInteractHoldCanceled(const FInputActionValue& Value);
    
	void OnQTEInteractStarted(const FInputActionValue& Value);
	void OnInteractProgress(const FInputActionValue& Value);
	void OnInteractCanceled(const FInputActionValue& Value);
	void OnToggleVoiceChat(const FInputActionValue& Value);
	void OnEscPressed(const FInputActionValue& Value);
	void OnQTEInput(const FInputActionValue& Value);

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
    
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Voice", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPrivateVoiceChatComponent> PrivateVoiceChatComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ending", meta = (AllowPrivateAccess = "true"))
	bool bMaliceEnding = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ending", meta = (AllowPrivateAccess = "true"))
	bool bNiceEnding = true;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	bool bIsDead = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Timed", meta = (AllowPrivateAccess = "true"))
	bool bHasActiveTimedKnife = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Timed", meta = (AllowPrivateAccess = "true"))
	float TimedKnifeRemainingSeconds = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Timed", meta = (AllowPrivateAccess = "true"))
	FName ActiveTimedKnifeItemId = NAME_None;
	
	// 무기 액터 참조 (애니메이션 재생 시 위치 참조용)
	UPROPERTY()
	AKnifeActor* KnifeActor = nullptr;

	UPROPERTY(EditAnywhere, Category="Weapon")
	TSubclassOf<AKnifeActor> KnifeActorClass;

	FTimerHandle TimedKnifeTimerHandle;

	UPROPERTY(VisibleAnywhere, Category = "Item|Test")
	int32 AutoAttackCount = 0;
};
