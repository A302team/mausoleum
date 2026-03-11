#include "Character/Dummy/DummyCharacter.h"

#include "Character/Components/CombatStatusComponent.h"
#include "Character/MyCharacter.h"
#include "Engine/Engine.h"
#include "GameData/ItemDefinition.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    void LogAndScreenDummy(const FString& Message, const FColor& Color = FColor::Yellow, const float Duration = 3.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, Duration, Color, Message);
        }
    }
}

ADummyCharacter::ADummyCharacter()
{
    PrimaryActorTick.bCanEverTick = false;
    CombatStatusComponent = CreateDefaultSubobject<UCombatStatusComponent>(TEXT("CombatStatusComponent"));
}

void ADummyCharacter::BeginPlay()
{
    Super::BeginPlay();
    SetupInitialShield();

    UWorld* World = GetWorld();
    if (!bEnableAutoAttack)
    {
        LogAndScreenDummy(TEXT("[DummyCharacter] Auto attack is disabled."), FColor::Orange, 3.0f);
        return;
    }

    if (bEnableAutoAttack && World && AutoAttackInterval > 0.f)
    {
        World->GetTimerManager().SetTimer(
            AutoAttackTimerHandle,
            this,
            &ADummyCharacter::TryAutoAttackPlayer,
            AutoAttackInterval,
            true,
            AutoAttackInterval
        );

        LogAndScreenDummy(
            FString::Printf(
                TEXT("[DummyCharacter] Auto attack started. interval=%.1fs range=%.0f"),
                AutoAttackInterval,
                AutoAttackRange
            ),
            FColor::Cyan,
            3.0f
        );
    }
}

float ADummyCharacter::TakeDamage(
    float DamageAmount,
    const FDamageEvent& DamageEvent,
    AController* EventInstigator,
    AActor* DamageCauser
)
{
    if (bIsDead)
    {
        return 0.f;
    }

    if (CombatStatusComponent && CombatStatusComponent->TryConsumeShieldToBlock())
    {
        LogAndScreenDummy(FString::Printf(TEXT("Blocked by Shield, remaining=%d"), CombatStatusComponent->ShieldBlockCount));
        return 0.f;
    }

    bIsDead = true;

    AMyCharacter* KillerCharacter = DamageCauser ? Cast<AMyCharacter>(DamageCauser) : nullptr;
    if (!KillerCharacter && EventInstigator)
    {
        KillerCharacter = Cast<AMyCharacter>(EventInstigator->GetPawn());
    }
    if (KillerCharacter)
    {
        KillerCharacter->NotifyKilledCharacter();
    }

    LogAndScreenDummy(TEXT("Dummy Dead"), FColor::Red, 4.0f);

    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);

    return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void ADummyCharacter::TryAutoAttackPlayer()
{
    if (bIsDead)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(World, 0);
    if (!PlayerCharacter)
    {
        // LogAndScreenDummy(TEXT("[DummyCharacter] Auto attack skipped: PlayerCharacter not found."), FColor::Orange, 1.0f);
        return;
    }

    const float Distance = FVector::Dist(PlayerCharacter->GetActorLocation(), GetActorLocation());
    if (Distance > AutoAttackRange)
    {
        // LogAndScreenDummy(
        //     FString::Printf(TEXT("[DummyCharacter] Auto attack skipped: out of range (%.0f/%.0f)"), Distance, AutoAttackRange),
        //     FColor::Silver,
        //     0.8f
        // );
        return;
    }

    UGameplayStatics::ApplyDamage(
        PlayerCharacter,
        AutoAttackDamage,
        GetController(),
        this,
        nullptr
    );

    LogAndScreenDummy(
        FString::Printf(TEXT("[DummyCharacter] Auto attack hit: %s"), *GetNameSafe(PlayerCharacter)),
        FColor::Cyan,
        1.0f
    );
}

void ADummyCharacter::SetupInitialShield()
{
    if (!CombatStatusComponent)
    {
        return;
    }

	int32 ShieldAmount = FMath::Max(0, InitialShieldStack);
	if (ShieldDef)
	{
		ShieldAmount *= FMath::Max(1, ShieldDef->Payload.BlockCount);
	}

    if (ShieldAmount <= 0)
    {
        return;
    }

    CombatStatusComponent->AddShield(ShieldAmount);
    LogAndScreenDummy(FString::Printf(TEXT("Shield applied x%d"), ShieldAmount));
}
