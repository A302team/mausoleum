#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DanceComponent.generated.h"

class UInputComponent;
class UInputAction;
class UAnimMontage;
class AMyCharacter;
struct FInputActionValue;

/**
 * 댄스 시스템 컴포넌트.
 * F1/F2/F3 키 입력 → Server RPC → Multicast 로 모든 클라이언트에 댄스 몽타주 동기화.
 * 단일 IA_Dance(Float) + IMC Scalar 모디파이어(1.0/2.0/3.0)로 세 키를 관리합니다.
 * 카메라 전환(3인칭 ↔ 1인칭)은 로컬 플레이어에만 적용.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302SHARED_API UDanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDanceComponent();

	/** AMyCharacter::SetupPlayerInputComponent 에서 호출 */
	void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// -------------------------------------------------------
	// Dance Montages – 블루프린트(BP_MyCharacter)에서 할당
	// -------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dance|Montage")
	UAnimMontage* DanceMontage1 = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dance|Montage")
	UAnimMontage* DanceMontage2 = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Dance|Montage")
	UAnimMontage* DanceMontage3 = nullptr;

	// -------------------------------------------------------
	// Input Action – 블루프린트(BP_MyCharacter)에서 할당
	// Float 타입 IA 1개로 F1/F2/F3 세 키를 관리합니다.
	// IMC에서 F1→Scalar(1.0), F2→Scalar(2.0), F3→Scalar(3.0) 으로 매핑하세요.
	// IA_ItemSelect(숫자 1~6)와 키가 다르므로 퀵슬롯과 충돌하지 않습니다.
	// -------------------------------------------------------
	UPROPERTY(EditDefaultsOnly, Category="Dance|Input")
	TObjectPtr<UInputAction> IA_Dance = nullptr;

	/** 댄스 중인지 여부 – 서버 권위적으로 관리되며 모든 클라이언트에 복제 */
	UPROPERTY(BlueprintReadOnly, Replicated, Category="Dance")
	bool bIsDancing = false;

protected:
	virtual void BeginPlay() override;

private:
	AMyCharacter* GetOwnerCharacter() const;

	// ----- 로컬 입력 핸들러 -----
	/** Scalar 모디파이어 값(1.0/2.0/3.0)을 읽어 댄스 인덱스(1~3)를 결정 */
	void OnDance(const FInputActionValue& Value);

	/** 로컬에서 춤 요청 → Server RPC 호출 */
	void RequestDance(int32 Index);

	/** 댄스 가능 여부 검사 (공격 중 / 방어 중 / 사망 시 불가) */
	bool CanDance() const;

	/** Index(1~3)에 해당하는 몽타주 반환 */
	UAnimMontage* GetDanceMontage(int32 Index) const;

	// ----- Network RPCs -----

	/** 클라이언트 → 서버: 댄스 시작 요청 */
	UFUNCTION(Server, Reliable)
	void Server_PlayDance(int32 Index);

	/** 클라이언트 → 서버: 댄스 중단 요청 (이동 감지 시) */
	UFUNCTION(Server, Reliable)
	void Server_StopDance();

	/** 서버 → 모든 클라이언트: 댄스 몽타주 재생 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayDance(int32 Index);

	/** 서버 → 모든 클라이언트: 댄스 몽타주 중단 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StopDance();

	// ----- 카메라 전환 (로컬 플레이어 전용) -----
	void SwitchToThirdPersonCamera();
	void RestoreFirstPersonCamera();

	/** 이동 감지 임계값 (cm/s) */
	static constexpr float MovementThreshold = 10.f;
};
