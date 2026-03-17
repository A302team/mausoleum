#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractComponent.generated.h"

UENUM(BlueprintType)
enum class EQTEDirection : uint8
{
	Up      UMETA(DisplayName = "Up"),
	Down    UMETA(DisplayName = "Down"),
	Left    UMETA(DisplayName = "Left"),
	Right   UMETA(DisplayName = "Right"),
	None    UMETA(DisplayName = "None")
};

class ACharacter;
class IA302CharacterBridge;
class UUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQTEStarted, const TArray<EQTEDirection>&, TargetKeys);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQTEProgressUpdated, int32, CurrentIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQTEEnded, bool, bWasSuccessful);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class A302SHARED_API UInteractComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
	// Called from MyCharacter::OnInteract input binding.
	
	// 홀드 입력
	bool HandleInteractHoldProgress(float DeltaTime); // 누르는 중 (Hold 게이지용)
	void HandleInteractHoldComplete();  // 완료 (결과 처리)
	void HandleInteractHoldCanceled();  // 취소 (초기화)
	
	// QTE
	void HandleInteractQTEStarted();
	bool ReceiveQTEInput(EQTEDirection InputDir);

	// Read by quick-slot flow right after HandleInteractInput.
	AActor* GetLastInteractedActor() const { return LastInteractedActor; }
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	float GetInteractionProgressRatio() const { return InteractionProgressRatio; }

	UPROPERTY(EditAnywhere, Category = "Interaction")
	float MaxHoldTime = 2.0f;
	
	UPROPERTY(EditAnywhere, Category = "Interaction")
	float InteractionDistance = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> InteractionWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> CrosshairWidgetClass;
    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UUserWidget> QTEWidgetClass;
    
	// 디버그 드로우 토글
	UPROPERTY(EditAnywhere, Category = "Interaction|Debug")
	bool bDrawDebug = true;
	
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnQTEStarted OnQTEStarted;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnQTEProgressUpdated OnQTEProgressUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnQTEEnded OnQTEEnded;

private:
	ACharacter* GetOwnerCharacter() const;
	IA302CharacterBridge* GetOwnerCharacterBridge() const;
	void CheckForInteractables();
	void ToggleHighlight(AActor* TargetActor, bool bIsOn) const;

	float InteractionProgressRatio = 0.0f;
    
	UPROPERTY()
	TObjectPtr<AActor> LastInteractableActor = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> LastInteractedActor = nullptr;

	UPROPERTY()
	TObjectPtr<UUserWidget> InteractionWidgetInstance = nullptr;

	UPROPERTY()
	TObjectPtr<UUserWidget> CrosshairWidgetInstance = nullptr;
    
	UPROPERTY()
	TObjectPtr<UUserWidget> QTEWidgetInstance = nullptr;
	
	UPROPERTY()
	TObjectPtr<ACharacter> CachedOwnerCharacter = nullptr;
	
	// -- QTE --
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|QTE", meta = (AllowPrivateAccess = "true"))
	TArray<EQTEDirection> TargetQTEKeys;
    
	int32 CurrentQTEIndex = 0;
	bool bIsQTEActive = false;
    
	void OnQTESuccess();
	void OnQTEFailure();
};
