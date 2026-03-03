// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/MyCharacter.h"
#include "Blueprint/UserWidget.h"

#include "DrawDebugHelpers.h"
#include "EnhancedInputComponent.h"
#include "GameData/ItemDefinition.h"
#include "GameData/ItemInstance.h"
#include "GameData/ItemTypes.h"
#include "InputAction.h"
#include "Interface/UsableItem.h"
#include "Kismet/GameplayStatics.h"
#include "Manager/ItemActionFactory.h"
#include "GamePlay/Items/BaseItem.h"
#include "Character/DummyCharacter.h"
#include "Engine/Engine.h"
#include "GameFramework/Controller.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogMyInput, Log, All);

namespace
{
    void LogAndScreen(const FString& Message, const FColor& Color = FColor::Yellow, const float Duration = 3.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, Duration, Color, Message);
        }
    }
}

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
	
	// 위젯 생성 및 뷰포트 추가
	if (InteractionWidgetClass)
	{
		InteractionWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), InteractionWidgetClass);
		if (InteractionWidgetInstance)
		{
			InteractionWidgetInstance->AddToViewport(10);
			// 처음엔 숨김 처리
			InteractionWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
		}
	}

    SetupKnifeForTest();
    FindAndWarpNearDummy();

    GetWorldTimerManager().SetTimerForNextTick([this]()
    {
        if (FirstAutoAttackDelay >= 0.f)
        {
            FTimerHandle TimerHandle;
            GetWorldTimerManager().SetTimer(
                TimerHandle,
                this,
                &AMyCharacter::ExecuteAutoKnifeAttack,
                FirstAutoAttackDelay,
                false
            );
        }

        if (SecondAutoAttackDelay >= 0.f)
        {
            FTimerHandle TimerHandle;
            GetWorldTimerManager().SetTimer(
                TimerHandle,
                this,
                &AMyCharacter::ExecuteAutoKnifeAttack,
                SecondAutoAttackDelay,
                false
            );
        }
    });
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
	
	if (IA_Interact)
	{
		EIC->BindAction(IA_Interact, ETriggerEvent::Started, this, &AMyCharacter::OnInteract);
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

void AMyCharacter::OnInteract(const FInputActionValue& Value)
{
	FVector Start = GetPawnViewLocation();
	FVector ForwardVector = GetViewRotation().Vector();
	FVector End = Start + (ForwardVector * InteractionDistance);
	
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	
	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
	{
		if (IInteractableInterface* Interactable = Cast<IInteractableInterface>(HitResult.GetActor()))
		{
			UE_LOG(LogTemp, Warning, TEXT("상호작용 키(F) 눌림! 대상: %s"), *HitResult.GetActor()->GetName());
			
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("상호작용 로직 성공적으로 실행됨!"));
			}
		}
	}
}

void AMyCharacter::CheckForInteractables()
{
	if (!IsLocallyControlled()) return;
	
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
	Params.AddIgnoredActor(this);
	
	AActor* CurrentHitActor = nullptr;

	// 라인트레이스 실행 (가장 먼저 부딪히는 Visibility 채널 물체 탐색)
	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
	{
		// 부딪힌 물체가 인터페이스를 가지고 있는지 확인
		if (IInteractableInterface* Interactable = Cast<IInteractableInterface>(HitResult.GetActor()))
		{
			CurrentHitActor = HitResult.GetActor();
			
			// 인터페이스의 GetInteractText()를 가져와서 화면에 출력
			FString DebugMsg = FString::Printf(TEXT("상호작용 가능: %s"), *Interactable->GetInteractText());
            
			// 뷰포트에 메시지 띄우기 (Key: -1은 매번 새로 띄움, 0은 덮어쓰기)
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(0, 0.1f, FColor::Cyan, DebugMsg);
			}
		}
	}
	
	if (CurrentHitActor != LastInteractableActor)
	{
		// 1. 기존 대상의 하이라이트 끄기
		if (LastInteractableActor)
		{
			ToggleHighlight(LastInteractableActor, false);
		}

		// 2. 새로운 대상의 하이라이트 켜기
		if (CurrentHitActor)
		{
			ToggleHighlight(CurrentHitActor, true);
		}
	
		if (InteractionWidgetInstance)
		{
			// 조준한 대상이 있으면 보이고, 없으면 숨김
			ESlateVisibility NewVisibility = CurrentHitActor ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
			InteractionWidgetInstance->SetVisibility(NewVisibility);
			
			if(CurrentHitActor) UE_LOG(LogTemp, Warning, TEXT("UI Visible!"));
		}
		
		// 3. 기록 갱신
		LastInteractableActor = CurrentHitActor;
	}
}

void AMyCharacter::ToggleHighlight(AActor* TargetActor, bool bIsOn)
{
	if (!TargetActor) return;

	// 액터가 가진 모든 메시(외형) 컴포넌트를 찾아서 커스텀 뎁스를 켬/끔
	TArray<UMeshComponent*> MeshComps;
	TargetActor->GetComponents<UMeshComponent>(MeshComps);

	for (UMeshComponent* MeshComp : MeshComps)
	{
		MeshComp->SetRenderCustomDepth(bIsOn);
	}
}

void AMyCharacter::SetupKnifeForTest()
{
    ItemActionFactory = NewObject<UItemActionFactory>(this);
    if (!ItemActionFactory)
    {
        UE_LOG(LogTemp, Error, TEXT("[MyCharacter] Failed to create UItemActionFactory."));
        return;
    }

    if (!KnifeDef)
    {
        UE_LOG(LogTemp, Warning, TEXT("[MyCharacter] KnifeDef is not set in editor."));
        return;
    }

    KnifeInstance = NewObject<UItemInstance>(this);
    if (!KnifeInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("[MyCharacter] Failed to create UItemInstance."));
        return;
    }

    KnifeInstance->Init(KnifeDef, FMath::Max(0, InitialKnifeStack));
    LogAndScreen(FString::Printf(TEXT("Knife Stack=%d set"), KnifeInstance->StackCount));

    KnifeLogic = ItemActionFactory->CreateLogic(this, KnifeInstance);
    if (!KnifeLogic)
    {
        UE_LOG(LogTemp, Error, TEXT("[MyCharacter] Failed to create KnifeLogic from KnifeDef."));
    }
}

void AMyCharacter::FindAndWarpNearDummy()
{
    CachedDummyCharacter = Cast<ADummyCharacter>(
        UGameplayStatics::GetActorOfClass(GetWorld(), ADummyCharacter::StaticClass())
    );

    if (!CachedDummyCharacter)
    {
        LogAndScreen(TEXT("[MyCharacter] DummyCharacter not found."), FColor::Orange);
        return;
    }

    const FVector DummyLocation = CachedDummyCharacter->GetActorLocation();
    const FVector WarpLocation = DummyLocation - CachedDummyCharacter->GetActorForwardVector() * AutoWarpDistanceToDummy;
    SetActorLocation(WarpLocation);
}

void AMyCharacter::ExecuteAutoKnifeAttack()
{
    ++AutoAttackCount;

    if (!KnifeLogic)
    {
        LogAndScreen(FString::Printf(TEXT("[MyCharacter] AutoAttack %d skipped: KnifeLogic is null."), AutoAttackCount), FColor::Orange);
        return;
    }

    if (!CachedDummyCharacter)
    {
        FindAndWarpNearDummy();
        if (!CachedDummyCharacter)
        {
            LogAndScreen(FString::Printf(TEXT("[MyCharacter] AutoAttack %d skipped: no dummy target."), AutoAttackCount), FColor::Orange);
            return;
        }
    }

    if (KnifeInstance && KnifeInstance->IsEmpty())
    {
        LogAndScreen(FString::Printf(TEXT("[MyCharacter] AutoAttack %d skipped: Knife stack is empty."), AutoAttackCount), FColor::Orange);
        return;
    }

    FItemTargetData TargetData;
    TargetData.TargetActor = CachedDummyCharacter;
    TargetData.TargetLocation = CachedDummyCharacter->GetActorLocation();

    const bool bUsed = IUsableItem::Execute_Use(KnifeLogic, this, TargetData);
    if (!bUsed)
    {
        LogAndScreen(FString::Printf(TEXT("[MyCharacter] AutoAttack %d failed: Use() returned false."), AutoAttackCount), FColor::Orange);
    }
}
