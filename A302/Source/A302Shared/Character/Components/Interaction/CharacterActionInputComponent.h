#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "CharacterActionInputComponent.generated.h"

class UInputComponent;
class UEnhancedInputComponent;
class UAnimMontage;
struct FInputActionValue;
class AMyCharacter;
class UQuickSlotComponent;
class UItemDefinition;

/**
 * 캐릭터의 모든 키 입력 바인딩(Move, Look, Jump, Attack 등) 처리와 
 * 이에 대응하는 로직(델리게이트 함수)을 전담하는 컴포넌트입니다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302SHARED_API UCharacterActionInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCharacterActionInputComponent();

	// AMyCharacter::SetupPlayerInputComponent 에서 호출해줍니다.
	void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent);

protected:
	virtual void BeginPlay() override;

private:
	AMyCharacter* GetOwnerCharacter() const;

	// 기존 AMyCharacter에 있던 입력 처리 함수들
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnJump(const FInputActionValue& Value);
	void OnJumpReleased(const FInputActionValue& Value);
	void OnFlyUp(const FInputActionValue& Value); // 탈출 후 자유 비행 중 스페이스 홀드 → 상승
	
	void OnItemSelect(const FInputActionValue& Value);
	void OnAttack(const FInputActionValue& Value);
	void OnUseSelectedItem(const FInputActionValue& Value);
	void OnToggleVoiceChat(const FInputActionValue& Value);
	void OnEscPressed(const FInputActionValue& Value);
	
	void OnQTEInput(const FInputActionValue& Value);

	bool TryGetSelectedItemContext(
		UQuickSlotComponent*& OutQuickSlotComp,
		UItemDefinition*& OutItemDefinition,
		UClass*& OutLogicClass
	) const;
	bool IsAttackItemLogicClass(const UClass* LogicClass) const;
	void HandlePrimaryItemUseFeedback(AMyCharacter* OwnerCharacter, UItemDefinition* UsedItemDefinition, int32 UsedSlotIndex) const;
	void TryHandleTargetedServerUse(
		AMyCharacter* OwnerCharacter,
		const UItemDefinition* UsedItemDefinition,
		bool bIsAttackItem
	) const;

	void ScheduleAttackCameraRecovery(AMyCharacter* OwnerCharacter, bool bIsCursedSword);
	void HandleAttackCameraRecovery();

	UPROPERTY(EditAnywhere, Category = "Camera", meta = (ClampMin = "0.0"))
	float DefaultAttackCameraRecoveryDelay = 0.6f;

	TWeakObjectPtr<AMyCharacter> PendingCameraRecoveryOwner;
	FTimerHandle AttackCameraRecoveryTimerHandle;

	void BeginAttackInputLock();
	void EndAttackInputLock();
	void HandleAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	bool bAttackInputLocked = false;
};
