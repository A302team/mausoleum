#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/StaticMeshComponent.h"
#include "GamePlay/Actor/WeaponActor.h"
#include "Interface/A302AnimationBridge.h"
#include "Interface/InteractableInterface.h"
#include "MyCharacter.generated.h"

class UInputAction;
class UInputMappingContext;
class UCombatStatusComponent;
class UItemDefinition;
class URewardDefinition;
class UItemManagerComponent;
class UUserWidget;
class ADummyCharacter;
class UKnifeAutoTestComponent;
class UInteractComponent;
class UItemTargetingComponent;
class UMaliceComponent;
class UQuickSlotComponent;
class AShieldActor;
class ABaseInteractable;
class UObject;
class UEquipmentComponent;
struct FInputActionValue;

UCLASS()
class A302SHARED_API AMyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMyCharacter();
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual bool IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const override;
	void NotifyKilledCharacter();
	void NotifyTimedKnifeAttackSucceeded();
	void RegisterTimedKillEvent(UObject* EventInstance);
	void ClearTimedKillEvent(UObject* EventInstance);
	void ForceDeathByPersonalEvent();
	void SetTimedKnifeAttackInProgress(bool bInProgress);

	UFUNCTION(BlueprintCallable, Category = "Health")
	bool IsDead() const;

	UFUNCTION(BlueprintPure, Category = "Interaction", meta = (DisplayName = "Get Interaction Progress Ratio"))
	float GetInteractionProgressRatio() const;

	UFUNCTION(BlueprintPure, Category = "Interaction", meta = (DisplayName = "Get Interaction Component"))
	UInteractComponent* GetInteractionComponentCompat() const;

	UFUNCTION(BlueprintCallable, Category = "Equipment")
	UEquipmentComponent* GetEquipmentComponent() const { return EquipmentComponent; }

	void SetQTEInputMode(bool bIsQTE);
	void BroadcastPublicMaliceAnnouncement(const FString& PlayerName, int32 MaliceCount);
    
    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnRequestPublicMaliceAnnouncement, const FString& /*PlayerName*/, int32 /*MaliceCount*/);
    FOnRequestPublicMaliceAnnouncement OnRequestPublicMaliceAnnouncement;

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

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

	UPROPERTY(EditAnywhere, Category = "Interaction")
	float InteractionDistance = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_VoiceChat = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_ItemSelect = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Attack = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_QTE_Input = nullptr;

	UFUNCTION(BlueprintImplementableEvent, Category = "Item|Action")
	void BP_OnPrimaryItemUsed(const UItemDefinition* ItemDefinition, int32 UseSlotIndexOneBased);

protected:
	virtual void BeginPlay() override;

	void CheckForInteractables();

	UPROPERTY(EditDefaultsOnly, Category = "Item|Definition")
	TObjectPtr<UItemDefinition> KnifeDef;

	UPROPERTY(EditDefaultsOnly, Category = "Item|Definition")
	TObjectPtr<UItemDefinition> ShieldDef;

	UPROPERTY(EditAnywhere, Category = "Item|Test", meta = (ClampMin = "0"))
	int32 InitialKnifeStack = 2;

	UPROPERTY(EditAnywhere, Category = "Item|Test", meta = (ClampMin = "0.0"))
	float FirstAutoAttackDelay = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Item|Test", meta = (ClampMin = "0.0"))
	float SecondAutoAttackDelay = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Item|Test", meta = (ClampMin = "0.0"))
	float AutoWarpDistanceToDummy = 120.f;

	void SetupKnifeForTest();
	void FindAndWarpNearDummy();
	void ExecuteAutoKnifeAttack();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;



private:
	UFUNCTION()
	void HandleShieldChanged(int32 NewCount);

	UFUNCTION()
	void HandleMaliceChanged(int32 NewCount);



	UPROPERTY()
	TObjectPtr<ADummyCharacter> CachedDummyCharacter;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Manager", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UItemManagerComponent> ItemManagerComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction|QTE", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInteractComponent> InteractionComponent = nullptr;

	// Legacy BP compatibility aliases kept during module/refactor migration.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction|QTE", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInteractComponent> InteractComp = nullptr;

	// Legacy BP compatibility aliases kept during module/refactor migration.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction|QTE", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UInteractComponent> InteractionQuickSlotComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCharacterActionInputComponent> CharacterActionInputComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|QuickSlot", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UQuickSlotComponent> QuickSlotComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Targeting", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UItemTargetingComponent> ItemTargetingComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatStatusComponent> CombatStatusComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Malice", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaliceComponent> MaliceComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Test", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UKnifeAutoTestComponent> KnifeAutoTestComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEquipmentComponent> EquipmentComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction|Reward", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCharacterRewardComponent> CharacterRewardComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction|Event", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UPlayerEventComponent> PlayerEventComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UCharacterHealthComponent> CharacterHealthComponent = nullptr;
};
