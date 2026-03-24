#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "CharacterHealthComponent.generated.h"

class AMyCharacter;
class AController;
class AActor;
struct FDamageEvent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302SHARED_API UCharacterHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCharacterHealthComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
	void HandleDead();
	void ForceDeathByPersonalEvent();

	bool IsDead() const { return bIsDead; }

protected:
	virtual void BeginPlay() override;

private:
	AMyCharacter* GetOwnerCharacter() const;

	void LogAndScreenHealthMessage(const FString& Message, const FColor& Color = FColor::Yellow, float Duration = 3.0f) const;

	UPROPERTY(ReplicatedUsing=OnRep_IsDead, VisibleAnywhere, Category = "Health", meta = (AllowPrivateAccess = "true"))
	bool bIsDead = false;

	UFUNCTION()
	void OnRep_IsDead();
};
