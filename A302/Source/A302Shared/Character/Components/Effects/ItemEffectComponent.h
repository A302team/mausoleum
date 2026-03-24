#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemEffectComponent.generated.h"

class UNiagaraSystem;
class USoundBase;
class UItemDefinition;
class UItemManagerComponent;
class UMaliceComponent;
class UCharacterRewardComponent;
class URewardDefinition;

/**
 * UItemEffectComponent
 * 
 * ItemManagerComponent의 DelegateAddItem을 감지하여 특정 아이템 획득 시,
 * MaliceComponent의 OnMaliceChanged를 감지하여 Malice 변경 시,
 * VFX와 사운드를 재생합니다.
 */

USTRUCT(BlueprintType)
struct FItemEffectData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	FName ItemID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	TObjectPtr<UNiagaraSystem> VFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	TObjectPtr<USoundBase> Sound;
};

USTRUCT(BlueprintType)
struct FMaliceEffectData
{
	GENERATED_BODY()

	/** Malice 카운트 (이 값일 때 효과 재생) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	int32 MaliceCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	TObjectPtr<UNiagaraSystem> VFX;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
	TObjectPtr<USoundBase> Sound;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302SHARED_API UItemEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UItemEffectComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "Item Effects")
	TArray<FItemEffectData> ItemEffects;

	UPROPERTY(EditAnywhere, Category = "Malice Effects")
	TArray<FMaliceEffectData> MaliceEffects;

private:
	UPROPERTY()
	TObjectPtr<UItemManagerComponent> CachedItemManager;

	UPROPERTY()
	TObjectPtr<UMaliceComponent> CachedMaliceComponent;

	UPROPERTY()
	TObjectPtr<UCharacterRewardComponent> CachedRewardComponent;

	void OnItemAdded(int32 SlotIndex, const UItemDefinition* ItemDefinition);
	
	UFUNCTION()
	void OnMaliceChanged(int32 NewCount);

	void OnRewardAcquired(const URewardDefinition* RewardDefinition);

	void PlayItemEffect(const FItemEffectData& EffectData);
	void PlayMaliceEffect(const FMaliceEffectData& EffectData, int32 MaliceCount);
	
	UItemManagerComponent* GetItemManager() const;
	UMaliceComponent* GetMaliceComponent() const;
	UCharacterRewardComponent* GetRewardComponent() const;
};
