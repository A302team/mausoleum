#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "KnifeAutoTestComponent.generated.h"

class ADummyCharacter;
class AMyCharacter;
class UBaseItem;
class UItemActionFactory;
class UItemDefinition;
class UItemInstance;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302_API UKnifeAutoTestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UKnifeAutoTestComponent();

	UFUNCTION(BlueprintCallable, Category = "Item|Test")
	void StartAutoKnifeTest();

	UPROPERTY(EditAnywhere, Category = "Item|Test")
	bool bEnableAutoKnifeTest = true;

	UPROPERTY(EditDefaultsOnly, Category = "Item|Definition")
	TObjectPtr<UItemDefinition> KnifeDef = nullptr;

	UPROPERTY(EditAnywhere, Category = "Item|Test", meta = (ClampMin = "0"))
	int32 InitialKnifeStack = 2;

	UPROPERTY(EditAnywhere, Category = "Item|Test", meta = (ClampMin = "0.0"))
	float FirstAutoAttackDelay = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Item|Test", meta = (ClampMin = "0.0"))
	float SecondAutoAttackDelay = 1.5f;

	UPROPERTY(EditAnywhere, Category = "Item|Test", meta = (ClampMin = "0.0"))
	float AutoWarpDistanceToDummy = 120.f;

private:
	AMyCharacter* GetOwnerCharacter() const;
	void SetupKnifeForTest();
	void FindAndWarpNearDummy();
	void ExecuteAutoKnifeAttack();

	UPROPERTY()
	TObjectPtr<UItemActionFactory> ItemActionFactory = nullptr;

	UPROPERTY()
	TObjectPtr<UItemInstance> KnifeInstance = nullptr;

	UPROPERTY()
	TObjectPtr<UBaseItem> KnifeLogic = nullptr;

	UPROPERTY()
	TObjectPtr<ADummyCharacter> CachedDummyCharacter = nullptr;

	int32 AutoAttackCount = 0;
};
