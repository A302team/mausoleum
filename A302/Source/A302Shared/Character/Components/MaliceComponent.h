#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "MaliceComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMaliceChanged, int32, NewCount);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302SHARED_API UMaliceComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMaliceComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(ReplicatedUsing = OnRep_MaliceCount, BlueprintReadOnly, Category = "Malice")
	int32 MaliceCount = 0;

	UPROPERTY(BlueprintAssignable, Category="Malice")
	FOnMaliceChanged OnMaliceChanged;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Malice")
	int32 ItemizeMalice = 0;
	
	UFUNCTION(BlueprintCallable, Category = "Malice")
	int32 GetRawMalice() const 
	{ 
		return MaliceCount - ItemizeMalice; 
	}

	UFUNCTION(BlueprintCallable, Category="Malice")
	void AddMalice(int32 Count);

	UFUNCTION(BlueprintCallable, Category="Malice")
	void ConsumeMalice(int32 Count);

	int32 GetMaliceCount() const { return FMath::Max(0, MaliceCount); }

	UFUNCTION(BlueprintCallable, Category = "Malice")
	void BroadcastPublicMaliceAnnouncement(const FString& PlayerName, int32 NewMaliceCount);

	// Multicast_ShowPublicMaliceAnnouncement is removed in favor of targeted RPCs via UA302RoomEventSubsystem.

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Malice|Ending")
	bool bMaliceEnding = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Malice|Ending")
	bool bNiceEnding = true;

private:
	UFUNCTION()
	void OnRep_MaliceCount();
};
