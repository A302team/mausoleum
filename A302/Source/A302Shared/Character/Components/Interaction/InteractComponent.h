#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
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
class AMyCharacter;
class AActor;
class UUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQTEStarted, const TArray<EQTEDirection>&, TargetKeys);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQTEProgressUpdated, int32, CurrentIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQTEEnded, bool, bWasSuccessful);

// 홀드 상호작용 (UI 통신용 델리게이트)
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHoldInteractionStarted, class AActor*, TargetActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHoldInteractionEnded);

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
	void OnInteractHoldProgress(const FInputActionValue& Value);
	void HandleInteractHoldStarted();   // 처음 누르는 순간 (UI 등)
	bool HandleInteractHoldProgress(float DeltaTime); // 누르는 중 (Hold 게이지용)
	void HandleInteractHoldComplete();  // 완료 (결과 처리)
	void OnInteractHoldCanceled(const FInputActionValue& Value);
	void HandleInteractHoldCanceled();  // 취소 (초기화)
	
	// QTE
	void OnQTEInteractStarted();
	void HandleInteractQTEStarted();
	void OnQTEInput(const FVector2D& InputVector);
	bool ReceiveQTEInput(EQTEDirection InputDir);
	void SetQTEInputMode(bool bIsQTE);
	
	void InteractionCompleteResult();

	// Read by quick-slot flow right after HandleInteractInput.
	AActor* GetLastInteractedActor() const { return LastInteractedActor; }
	
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	float GetInteractionProgressRatio() const { return InteractionProgressRatio; }

	// -- Sync Hold Progress --
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SyncHoldProgress(AActor* InteractTarget, float DeltaTime);

	float AccumulatedHoldSyncTime = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Interaction")
	float MaxHoldTime = 2.0f;
	
	UPROPERTY(EditAnywhere, Category = "Interaction")
	float InteractionDistance = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSoftClassPtr<UUserWidget> InteractionWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSoftClassPtr<class UStatueProgressWidget> StatueProgressWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSoftClassPtr<UUserWidget> CrosshairWidgetClass;
    
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSoftClassPtr<UUserWidget> QTEWidgetClass;
    
	// 디버그 드로우 토글
	UPROPERTY(EditAnywhere, Category = "Interaction|Debug")
	bool bDrawDebug = true;

	UPROPERTY(EditAnywhere, Category = "Interaction|Highlight", meta = (ClampMin = "0.0"))
	float NearbyHighlightRadius = 600.0f;

	UPROPERTY(EditAnywhere, Category = "Interaction|Highlight", meta = (ClampMin = "0", ClampMax = "255"))
	int32 NearbyHighlightStencilValue = 1;

	UPROPERTY(EditAnywhere, Category = "Interaction|Highlight", meta = (ClampMin = "0", ClampMax = "255"))
	int32 FocusedHighlightStencilValue = 2;

	UPROPERTY(EditAnywhere, Category = "Interaction|Highlight", meta = (ClampMin = "0", ClampMax = "255"))
	int32 NearbyAndFocusedHighlightStencilValue = 3;
	
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnQTEStarted OnQTEStarted;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnQTEProgressUpdated OnQTEProgressUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnQTEEnded OnQTEEnded;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnHoldInteractionStarted OnHoldInteractionStarted;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|Events")
	FOnHoldInteractionEnded OnHoldInteractionEnded;

private:
	bool TryInitializeLocalUIWidgets();
	ACharacter* GetOwnerCharacter() const;
	AMyCharacter* GetOwnerCharacterBridge() const;
	void CheckForInteractables();
	void UpdateNearbyHighlights();
	void RefreshHighlightState(AActor* TargetActor) const;
	void SetHighlightVisual(AActor* TargetActor, bool bIsOn, int32 StencilValue) const;

	float InteractionProgressRatio = 0.0f;
    
	UPROPERTY()
	TObjectPtr<AActor> LastInteractableActor = nullptr;

	UPROPERTY()
	TObjectPtr<AActor> LastInteractedActor = nullptr;

	UPROPERTY()
	TObjectPtr<UUserWidget> InteractionWidgetInstance = nullptr;

	UPROPERTY()
	TObjectPtr<class UStatueProgressWidget> StatueProgressWidgetInstance = nullptr;

	UPROPERTY()
	TObjectPtr<UUserWidget> CrosshairWidgetInstance = nullptr;
    
	UPROPERTY()
	TObjectPtr<UUserWidget> QTEWidgetInstance = nullptr;
	
	UPROPERTY()
	TObjectPtr<ACharacter> CachedOwnerCharacter = nullptr;

	bool bLocalUIInitialized = false;

	TSet<TWeakObjectPtr<AActor>> NearbyHighlightedActors;
	
	// -- QTE --
	UPROPERTY(BlueprintReadOnly, Category = "Interaction|QTE", meta = (AllowPrivateAccess = "true"))
	TArray<EQTEDirection> TargetQTEKeys;
    
	int32 CurrentQTEIndex = 0;
	bool bIsQTEActive = false;
    
	void OnQTESuccess();
	void OnQTEFailure();
};
