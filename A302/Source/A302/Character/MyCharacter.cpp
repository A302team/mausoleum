// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/MyCharacter.h"

#include "DrawDebugHelpers.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "GameFramework/Controller.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyInput, Log, All);

// Sets default values
AMyCharacter::AMyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	CheckForInteractables();
}

// Called to bind functionality to input
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UE_LOG(LogMyInput, Warning, TEXT("[Input] SetupPlayerInputComponent called. PC=%s"),
        *GetNameSafe(PlayerInputComponent));

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    UE_LOG(LogMyInput, Warning, TEXT("[Input] EnhancedInputComponent cast: %s"),
        EIC ? TEXT("OK") : TEXT("FAILED"));

    UE_LOG(LogMyInput, Warning, TEXT("[Input] IA_Move=%s IA_Look=%s IA_Jump=%s"),
        *GetNameSafe(IA_Move), *GetNameSafe(IA_Look), *GetNameSafe(IA_Jump));

    if (!EIC) return;

    if (IA_Move)
        EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AMyCharacter::OnMove);

    if (IA_Look)
        EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &AMyCharacter::OnLook);

    if (IA_Jump)
    {
        EIC->BindAction(IA_Jump, ETriggerEvent::Started,   this, &AMyCharacter::OnJump);
        EIC->BindAction(IA_Jump, ETriggerEvent::Completed, this, &AMyCharacter::OnJumpReleased);
        EIC->BindAction(IA_Jump, ETriggerEvent::Canceled,  this, &AMyCharacter::OnJumpReleased);
    }
}

void AMyCharacter::OnMove(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>(); // IA_Move: Axis2D

	// 컨트롤러 yaw 기준으로 전/우 벡터 계산
	const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);

	// X=좌우, Y=앞뒤
	AddMovementInput(Forward, Axis.Y);
	AddMovementInput(Right, Axis.X);

}

void AMyCharacter::OnLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>(); // IA_Look: Axis2D

	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);

}

void AMyCharacter::OnJump(const FInputActionValue& Value)
{
    Jump();
	
}

void AMyCharacter::OnJumpReleased(const FInputActionValue& Value)
{
    StopJumping();

}

void AMyCharacter::CheckForInteractables()
{
	//시작점: 카메라 위치
	FVector Start = GetPawnViewLocation();
    
	// 방향: 캐릭터가 바라보는 정면 방향
	FVector ForwardVector = GetViewRotation().Vector();
    
	// 끝점: 시작점 + (방향 * 거리)
	FVector End = Start + (ForwardVector * InteractionDistance);
	
	DrawDebugLine(
		GetWorld(),
		Start,
		End,
		FColor::Red,  
		false,        // 한 프레임만 유지 (Tick 함수에서 매 프레임 호출되므로 false가 맞음)
		-1.0f,        // 화면 유지 시간 (false일 땐 무시됨)
		0,            // 우선순위
		0.0f          // 선의 두께
	);
	
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this); // 나 자신은 무시

	// 라인트레이스 실행 (가장 먼저 부딪히는 Visibility 채널 물체 탐색)
	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
	{
		// 부딪힌 물체가 인터페이스를 가지고 있는지 확인
		if (IInteractableInterface* Interactable = Cast<IInteractableInterface>(HitResult.GetActor()))
		{
			// 인터페이스의 GetInteractText()를 가져와서 화면에 출력
			FString DebugMsg = FString::Printf(TEXT("상호작용 가능: %s"), *Interactable->GetInteractText());
            
			// 뷰포트에 메시지 띄우기 (Key: -1은 매번 새로 띄움, 0은 덮어쓰기)
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(0, 0.1f, FColor::Cyan, DebugMsg);
			}
			return;
		}
	}
}