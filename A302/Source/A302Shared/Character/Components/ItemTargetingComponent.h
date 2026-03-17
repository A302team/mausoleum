#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemTargetingComponent.generated.h"

class ACharacter;
class UItemManagerComponent;
class UQuickSlotComponent;
class UItemDefinition;
struct FItemTargetData;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302SHARED_API UItemTargetingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UItemTargetingComponent();
	virtual void BeginPlay() override;
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction
	) override;

	AActor* FindTargetActorForUse(FVector& OutTargetLocation) const;
	bool TryBuildTargetDataForUse(
		const UItemDefinition* ItemDefinition,
		FItemTargetData& OutTargetData,
		bool bForceTargetActor = false
	) const;
	void UpdateAttackRangeDebugState();

private:
	ACharacter* GetOwnerCharacter() const;
	UItemManagerComponent* GetItemManager() const;
	UQuickSlotComponent* GetQuickSlot() const;

	void LogAndScreenMessage(
		const FString& Message,
		const FColor& Color = FColor::Yellow,
		float Duration = 1.2f
	) const;

	UPROPERTY()
	TObjectPtr<UItemManagerComponent> ItemManagerComponent = nullptr;

	UPROPERTY()
	TObjectPtr<UQuickSlotComponent> QuickSlotComponent = nullptr;

	UPROPERTY(EditAnywhere, Category = "Item|Targeting", meta = (ClampMin = "1.0"))
	float AttackTraceDistance = 300.0f;

	UPROPERTY(EditAnywhere, Category = "Item|Targeting")
	bool bEnableAttackRangeDebug = true;

	bool bWasAttackTargetInRange = false;
};
