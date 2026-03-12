#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "MaliceComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMaliceChanged, int32, NewCount);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302_API UMaliceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMaliceComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_MaliceCount, BlueprintReadOnly, Category="Malice")
	int32 MaliceCount = 0;

	UPROPERTY(BlueprintAssignable, Category="Malice")
	FOnMaliceChanged OnMaliceChanged;

	UFUNCTION(BlueprintCallable, Category="Malice")
	void AddMalice(int32 Count);

	UFUNCTION(BlueprintCallable, Category="Malice")
	void ConsumeMalice(int32 Count);

private:
	UFUNCTION()
	void OnRep_MaliceCount();
};
