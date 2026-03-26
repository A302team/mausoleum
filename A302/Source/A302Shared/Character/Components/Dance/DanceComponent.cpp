#include "Character/Components/Dance/DanceComponent.h"
#include "Character/MyCharacter.h"
#include "Animation/MyAnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"

UDanceComponent::UDanceComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UDanceComponent::BeginPlay()
{
	Super::BeginPlay();
}

// ------------------------------------------------------------
// 복제 속성 등록
// ------------------------------------------------------------
void UDanceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UDanceComponent, bIsDancing);
}

// ------------------------------------------------------------
// 입력 바인딩 (AMyCharacter::SetupPlayerInputComponent 에서 호출)
// ------------------------------------------------------------
void UDanceComponent::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC) return;

	// IA_Dance(Float) 1개에 F1/F2/F3을 Scalar 모디파이어(1.0/2.0/3.0)로 매핑합니다.
	if (IA_Dance) EIC->BindAction(IA_Dance, ETriggerEvent::Started, this, &UDanceComponent::OnDance);
}

// ------------------------------------------------------------
// Tick: 이동 감지 시 댄스 중단
// ------------------------------------------------------------
void UDanceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsDancing) return;

	AMyCharacter* Owner = GetOwnerCharacter();
	if (!Owner) return;

	FVector Velocity = Owner->GetVelocity();
	Velocity.Z = 0.f;

	if (Velocity.SizeSquared() > MovementThreshold * MovementThreshold)
	{
		if (Owner->HasAuthority())
		{
			// 서버: 직접 중단 처리 후 모든 클라이언트에 전파
			bIsDancing = false;
			Multicast_StopDance();
		}
		else if (Owner->IsLocallyControlled())
		{
			// 클라이언트: 서버에 중단 요청
			Server_StopDance();
		}
	}
}

// ------------------------------------------------------------
// 내부 헬퍼
// ------------------------------------------------------------
AMyCharacter* UDanceComponent::GetOwnerCharacter() const
{
	return Cast<AMyCharacter>(GetOwner());
}

UAnimMontage* UDanceComponent::GetDanceMontage(int32 Index) const
{
	switch (Index)
	{
	case 1: return DanceMontage1;
	case 2: return DanceMontage2;
	case 3: return DanceMontage3;
	default: return nullptr;
	}
}

bool UDanceComponent::CanDance() const
{
	const AMyCharacter* Owner = GetOwnerCharacter();
	if (!Owner) return false;
	if (Owner->IsDead()) return false;

	const UMyAnimInstance* AnimInst = Cast<UMyAnimInstance>(Owner->GetMesh()->GetAnimInstance());
	if (!AnimInst) return false;
	if (AnimInst->bIsAttacking || AnimInst->bIsBlocking) return false;

	return true;
}

// ------------------------------------------------------------
// 로컬 입력 핸들러
// ------------------------------------------------------------
void UDanceComponent::OnDance(const FInputActionValue& Value)
{
	// IMC에서 F1→Scalar(1.0), F2→Scalar(2.0), F3→Scalar(3.0)으로 매핑
	const int32 Index = FMath::RoundToInt(Value.Get<float>());
	if (Index >= 1 && Index <= 3)
	{
		RequestDance(Index);
	}
}

void UDanceComponent::RequestDance(int32 Index)
{
	// 로컬 사전 검사(반응성 향상)
	if (!CanDance()) return;

	// 이미 같은 댄스 중이면 무시
	if (bIsDancing) return;

	Server_PlayDance(Index);
}

// ------------------------------------------------------------
// Server RPC – 댄스 시작
// ------------------------------------------------------------
void UDanceComponent::Server_PlayDance_Implementation(int32 Index)
{
	if (!CanDance()) return;
	if (bIsDancing) return;

	bIsDancing = true;
	Multicast_PlayDance(Index);
}

// ------------------------------------------------------------
// Server RPC – 댄스 중단 (클라이언트 이동 감지 시 호출)
// ------------------------------------------------------------
void UDanceComponent::Server_StopDance_Implementation()
{
	if (!bIsDancing) return;

	bIsDancing = false;
	Multicast_StopDance();
}

// ------------------------------------------------------------
// Multicast RPC – 모든 클라이언트에서 댄스 몽타주 재생
// ------------------------------------------------------------
void UDanceComponent::Multicast_PlayDance_Implementation(int32 Index)
{
	AMyCharacter* Owner = GetOwnerCharacter();
	if (!Owner) return;

	UMyAnimInstance* AnimInst = Cast<UMyAnimInstance>(Owner->GetMesh()->GetAnimInstance());
	if (AnimInst)
	{
		AnimInst->PlayDanceMontage(Index);
	}

	// 카메라 전환은 로컬 플레이어에만 적용
	if (Owner->IsLocallyControlled())
	{
		SwitchToThirdPersonCamera();
	}
}

// ------------------------------------------------------------
// Multicast RPC – 모든 클라이언트에서 댄스 몽타주 중단
// ------------------------------------------------------------
void UDanceComponent::Multicast_StopDance_Implementation()
{
	AMyCharacter* Owner = GetOwnerCharacter();
	if (!Owner) return;

	UMyAnimInstance* AnimInst = Cast<UMyAnimInstance>(Owner->GetMesh()->GetAnimInstance());
	if (AnimInst)
	{
		AnimInst->StopDanceMontage();
	}

	// 카메라 복원은 로컬 플레이어에만 적용
	if (Owner->IsLocallyControlled())
	{
		RestoreFirstPersonCamera();
	}
}

// ------------------------------------------------------------
// 카메라 전환 – 로컬 플레이어 전용
// ------------------------------------------------------------
void UDanceComponent::SwitchToThirdPersonCamera()
{
	AMyCharacter* Owner = GetOwnerCharacter();
	if (!Owner) return;

	Owner->SetCameraViewMode(EA302CameraViewMode::ThirdPerson);
}

void UDanceComponent::RestoreFirstPersonCamera()
{
	AMyCharacter* Owner = GetOwnerCharacter();
	if (!Owner) return;

	Owner->SetCameraViewMode(EA302CameraViewMode::FirstPersonChest);
}
