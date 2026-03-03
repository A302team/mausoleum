#include "Character/DummyCharacter.h"

#include "Character/Components/CombatStatusComponent.h"
#include "Engine/Engine.h"
#include "GameData/ItemDefinition.h"
#include "GameData/ItemInstance.h"
#include "GameData/ItemTypes.h"
#include "GamePlay/Items/BaseItem.h"
#include "Interface/UsableItem.h"
#include "Manager/ItemActionFactory.h"

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
    LogAndScreenDummy(TEXT("Dummy Dead"), FColor::Red, 4.0f);

    SetActorEnableCollision(false);
    SetActorHiddenInGame(true);

    return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

void ADummyCharacter::SetupInitialShield()
{
    ItemActionFactory = NewObject<UItemActionFactory>(this);
    if (!ItemActionFactory)
    {
        UE_LOG(LogTemp, Error, TEXT("[DummyCharacter] Failed to create UItemActionFactory."));
        return;
    }

    if (!ShieldDef)
    {
        LogAndScreenDummy(TEXT("[DummyCharacter] ShieldDef is not set in editor."), FColor::Orange);
        return;
    }

    ShieldInstance = NewObject<UItemInstance>(this);
    if (!ShieldInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("[DummyCharacter] Failed to create UItemInstance."));
        return;
    }

    ShieldInstance->Init(ShieldDef, FMath::Max(0, InitialShieldStack));
    ShieldLogic = ItemActionFactory->CreateLogic(this, ShieldInstance);
    if (!ShieldLogic)
    {
        UE_LOG(LogTemp, Error, TEXT("[DummyCharacter] Failed to create ShieldLogic from ShieldDef."));
        return;
    }

    const int32 UseCount = ShieldInstance->StackCount;
    int32 AppliedCount = 0;

    FItemTargetData TargetData;
    for (int32 i = 0; i < UseCount; ++i)
    {
        if (IUsableItem::Execute_Use(ShieldLogic, this, TargetData))
        {
            ++AppliedCount;
        }
    }

    if (AppliedCount == 1)
    {
        LogAndScreenDummy(TEXT("Shield applied once"));
    }
    else
    {
        LogAndScreenDummy(FString::Printf(TEXT("Shield applied x%d"), AppliedCount));
    }
}
