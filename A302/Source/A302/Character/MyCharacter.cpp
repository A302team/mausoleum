// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/MyCharacter.h"

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
