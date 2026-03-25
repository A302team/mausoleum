#include "Character/Components/Interaction/InteractComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Components/MeshComponent.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Interface/InteractableInterface.h"
#include "Character/MyCharacter.h"
#include "Character/Components/Interaction/CharacterRewardComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Interface/A302AnimationBridge.h"
#include "Object/BaseInteractable.h"
#include "A302RuntimeGuards.h"
#include "Character/Components/TraceHelper.h"
#include "UI/StatueProgressWidget.h"
#include "Object/StatueInteractable.h"

UInteractComponent::UInteractComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UInteractComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// 시작할 때 한 번만 오너 캐릭터를 찾아 캐싱합니다.
	CachedOwnerCharacter = Cast<ACharacter>(GetOwner());

	if (!A302RuntimeGuards::ShouldRunClientOnlyLogic(this))
	{
		return;
	}

	TryInitializeLocalUIWidgets();
}

bool UInteractComponent::TryInitializeLocalUIWidgets()
{
	if (bLocalUIInitialized)
	{
		return true;
	}

	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled())
	{
		return false;
	}

	APlayerController* LocalPC = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!LocalPC || !LocalPC->IsLocalController())
	{
		return false;
	}

	if (!InteractionWidgetClass.IsNull() && !InteractionWidgetInstance)
	{
		UClass* LoadedClass = InteractionWidgetClass.LoadSynchronous();
		if (LoadedClass)
		{
			InteractionWidgetInstance = CreateWidget<UUserWidget>(LocalPC, LoadedClass);
			if (InteractionWidgetInstance)
			{
				InteractionWidgetInstance->AddToViewport(10);
				InteractionWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}

	if (!StatueProgressWidgetClass.IsNull() && !StatueProgressWidgetInstance)
	{
		UClass* LoadedClass = StatueProgressWidgetClass.LoadSynchronous();
		if (LoadedClass)
		{
			StatueProgressWidgetInstance = CreateWidget<UStatueProgressWidget>(LocalPC, LoadedClass);
			if (StatueProgressWidgetInstance)
			{
				StatueProgressWidgetInstance->AddToViewport(15);
				StatueProgressWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}

	if (!CrosshairWidgetClass.IsNull() && !CrosshairWidgetInstance)
	{
		UClass* LoadedClass = CrosshairWidgetClass.LoadSynchronous();
		if (LoadedClass)
		{
			CrosshairWidgetInstance = CreateWidget<UUserWidget>(LocalPC, LoadedClass);
			if (CrosshairWidgetInstance)
			{
				CrosshairWidgetInstance->AddToViewport(20);
			}
		}
	}

	if (!QTEWidgetClass.IsNull() && !QTEWidgetInstance)
	{
		UClass* LoadedClass = QTEWidgetClass.LoadSynchronous();
		if (LoadedClass)
		{
			QTEWidgetInstance = CreateWidget<UUserWidget>(LocalPC, LoadedClass);
			if (QTEWidgetInstance)
			{
				QTEWidgetInstance->AddToViewport(50);
				QTEWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
				UE_LOG(LogTemp, Log, TEXT("[InteractComponent] QTE widget initialized for local player."));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[InteractComponent] Failed to load QTE widget class."));
		}
	}

	bLocalUIInitialized = true;
	return true;
}

void UInteractComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bLocalUIInitialized)
	{
		TryInitializeLocalUIWidgets();
	}

	CheckForInteractables();
	UpdateNearbyHighlights();
}

ACharacter* UInteractComponent::GetOwnerCharacter() const
{
	return CachedOwnerCharacter;
}

AMyCharacter* UInteractComponent::GetOwnerCharacterBridge() const
{
	return Cast<AMyCharacter>(CachedOwnerCharacter.Get());
}

void UInteractComponent::CheckForInteractables()
{
	// 상호작용 홀드 중일 때는 시선을 돌려도 타겟팅이 유지되도록 레이캐스트를 중단합니다.
	if (bIsHoldingInteraction)
	{
		return;
	}

	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled())
	{
		return;
	}

	FVector Start = OwnerCharacter->GetPawnViewLocation();
	FVector ForwardVector = OwnerCharacter->GetViewRotation().Vector();
	TraceHelper::TryGetCrosshairTrace(OwnerCharacter, Start, ForwardVector);
	const FVector End = Start + (ForwardVector * InteractionDistance);

	// 디버그 옵션이 켜져 있을 때만 라인을 그립니다.
	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, -1.0f, 0, 0.0f);
	}

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerCharacter);

	AActor* CurrentHitActor = nullptr;

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
	{
		if (IInteractableInterface* Interactable = Cast<IInteractableInterface>(HitResult.GetActor()))
		{
			CurrentHitActor = HitResult.GetActor();

			// 디버그 옵션이 켜져 있을 때만 텍스트를 출력합니다.
			if (bDrawDebug && GEngine)
			{
				FString TypeStr = (Interactable->GetInteractType() == EInteractType::Hold) ? TEXT("Hold") : TEXT("QTE");
				const FString DebugMsg = FString::Printf(TEXT("[%s] Interactable: %s"), *TypeStr, *Interactable->GetInteractText());
				GEngine->AddOnScreenDebugMessage(0, 0.1f, FColor::Cyan, DebugMsg);
			}
		}
	}

	if (CurrentHitActor != LastInteractableActor)
	{
		InteractionProgressRatio = 0.0f;

		AActor* PreviousFocusedActor = LastInteractableActor;
		LastInteractableActor = CurrentHitActor;

		if (PreviousFocusedActor)
		{
			RefreshHighlightState(PreviousFocusedActor);
		}

		if (CurrentHitActor)
		{
			RefreshHighlightState(CurrentHitActor);
		}

		if (InteractionWidgetInstance)
		{
			const ESlateVisibility NewVisibility = CurrentHitActor ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
			InteractionWidgetInstance->SetVisibility(NewVisibility);
		}
	}
}

void UInteractComponent::UpdateNearbyHighlights()
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled() || !GetWorld())
	{
		return;
	}

	TSet<TWeakObjectPtr<AActor>> NewNearbyActors;

	TArray<FOverlapResult> OverlapResults;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);

	const bool bHasOverlap = GetWorld()->OverlapMultiByObjectType(
		OverlapResults,
		OwnerCharacter->GetActorLocation(),
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(NearbyHighlightRadius),
		QueryParams
	);

	if (bHasOverlap)
	{
		for (const FOverlapResult& OverlapResult : OverlapResults)
		{
			ABaseInteractable* Interactable = Cast<ABaseInteractable>(OverlapResult.GetActor());
			if (!IsValid(Interactable))
			{
				continue;
			}

			NewNearbyActors.Add(Interactable);
		}
	}

	const TSet<TWeakObjectPtr<AActor>> PreviousNearbyActors = NearbyHighlightedActors;
	NearbyHighlightedActors = MoveTemp(NewNearbyActors);

	for (const TWeakObjectPtr<AActor>& PreviousActor : PreviousNearbyActors)
	{
		if (!PreviousActor.IsValid() || NearbyHighlightedActors.Contains(PreviousActor))
		{
			continue;
		}

		RefreshHighlightState(PreviousActor.Get());
	}

	for (const TWeakObjectPtr<AActor>& NewActor : NearbyHighlightedActors)
	{
		if (!NewActor.IsValid() || PreviousNearbyActors.Contains(NewActor))
		{
			continue;
		}

		RefreshHighlightState(NewActor.Get());
	}
}

void UInteractComponent::RefreshHighlightState(AActor* TargetActor) const
{
	if (!TargetActor)
	{
		return;
	}

	const bool bIsNearby = NearbyHighlightedActors.Contains(TWeakObjectPtr<AActor>(TargetActor));
	const bool bIsFocused = (TargetActor == LastInteractableActor);

	if (!bIsNearby && !bIsFocused)
	{
		SetHighlightVisual(TargetActor, false, 0);
		return;
	}

	int32 StencilValue = FocusedHighlightStencilValue;
	if (bIsNearby && bIsFocused)
	{
		StencilValue = NearbyAndFocusedHighlightStencilValue;
	}
	else if (bIsNearby)
	{
		StencilValue = NearbyHighlightStencilValue;
	}

	SetHighlightVisual(TargetActor, true, StencilValue);
}

void UInteractComponent::SetHighlightVisual(AActor* TargetActor, bool bIsOn, int32 StencilValue) const
{
	if (!TargetActor)
	{
		return;
	}

	TArray<UMeshComponent*> MeshComps;
	TargetActor->GetComponents<UMeshComponent>(MeshComps);

	for (UMeshComponent* MeshComp : MeshComps)
	{
		MeshComp->SetRenderCustomDepth(bIsOn);
		MeshComp->SetCustomDepthStencilValue(StencilValue);
	}
}

bool UInteractComponent::HandleInteractHoldProgress(float DeltaTime)
{
	if (!LastInteractableActor) return false;

	if (IInteractableInterface* Interactable = Cast<IInteractableInterface>(LastInteractableActor))
	{
		if (Interactable->GetInteractType() == EInteractType::Hold)
		{
			if (InteractionProgressRatio == 0.0f)
			{
				HandleInteractHoldStarted();
			}

			AccumulatedHoldSyncTime += DeltaTime;
			if (AccumulatedHoldSyncTime >= 0.25f)
			{
				Server_SyncHoldProgress(LastInteractableActor, AccumulatedHoldSyncTime);
				AccumulatedHoldSyncTime = 0.0f;
			}

			InteractionProgressRatio += (DeltaTime / MaxHoldTime);
          
			if (InteractionProgressRatio >= 1.0f)
			{
				InteractionProgressRatio = 0.0f;
				HandleInteractHoldComplete();
				return true;
			}
		}
	}
	return false;
}

void UInteractComponent::HandleInteractHoldStarted()
{
	bIsHoldingInteraction = true;
	
	// 석상을 홀드할 때만 기존 동그란 상호작용 UI를 숨깁니다. 다른 일반 아이템은 그대로 두게 합니다.
	if (InteractionWidgetInstance && LastInteractableActor && LastInteractableActor->IsA(AStatueInteractable::StaticClass()))
	{
		InteractionWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
	}

	if (LastInteractableActor)
	{
		OnHoldInteractionStarted.Broadcast(LastInteractableActor);
	}

	// Statue 상호작용 대상일 때만 로컬 캐릭터에서 몽타주 재생 + 3인칭 전환
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (OwnerCharacter && OwnerCharacter->IsLocallyControlled() && Cast<AStatueInteractable>(LastInteractableActor))
	{
		if (IA302AnimationBridge* Anim = Cast<IA302AnimationBridge>(OwnerCharacter->GetMesh()->GetAnimInstance()))
		{
			Anim->PlayStatueInteractAnimation();
		}

		// 3인칭 카메라로 전환
		if (AMyCharacter* MyChar = Cast<AMyCharacter>(OwnerCharacter))
		{
			MyChar->SetCameraViewMode(EA302CameraViewMode::ThirdPerson);
			UE_LOG(LogTemp, Warning, TEXT("[InteractComponent] Statue 상호작용 시작 - 3인칭 카메라 전환"));
		}
	}
}

void UInteractComponent::HandleInteractHoldComplete()
{
	bIsHoldingInteraction = false;
	
	// 숨겼던 기본 UI를 다시 복구해줍니다 (석상 상호작용이 끝난 후 다시 바라볼 때 표시)
	if (InteractionWidgetInstance && LastInteractableActor)
	{
		InteractionWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	}

	LastInteractedActor = nullptr;
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || !LastInteractableActor) return;

	if (IInteractableInterface* Interactable = Cast<IInteractableInterface>(LastInteractableActor))
	{
		if (Interactable->GetInteractType() == EInteractType::Hold)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Interaction] Hold 상호작용 성공!"));
			if (AccumulatedHoldSyncTime > 0.0f)
			{
				Server_SyncHoldProgress(LastInteractableActor, AccumulatedHoldSyncTime);
				AccumulatedHoldSyncTime = 0.0f;
			}
			Interactable->Interact(OwnerCharacter);
			LastInteractedActor = LastInteractableActor;
		}
	}

	OnHoldInteractionEnded.Broadcast();
}

void UInteractComponent::HandleInteractHoldCanceled()
{
	bIsHoldingInteraction = false;

	// 상호작용 취소 시에도 숨겨놨던 기본 UI를 복구
	if (InteractionWidgetInstance && LastInteractableActor)
	{
		InteractionWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	}

	InteractionProgressRatio = 0.0f;
	AccumulatedHoldSyncTime = 0.0f;

	OnHoldInteractionEnded.Broadcast();

	// Statue 상호작용 몽타주 중지 + 1인칭 복귀 (로컬 캐릭터에서만)
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (OwnerCharacter && OwnerCharacter->IsLocallyControlled())
	{
		if (IA302AnimationBridge* Anim = Cast<IA302AnimationBridge>(OwnerCharacter->GetMesh()->GetAnimInstance()))
		{
			Anim->StopStatueInteractAnimation();
		}

		// 1인칭 카메라로 복귀
		if (AMyCharacter* MyChar = Cast<AMyCharacter>(OwnerCharacter))
		{
			MyChar->SetCameraViewMode(EA302CameraViewMode::FirstPersonChest);
			UE_LOG(LogTemp, Warning, TEXT("[InteractComponent] Statue 상호작용 취소 - 1인칭 카메라 복귀"));
		}
	}
}

void UInteractComponent::OnInteractHoldProgress(const FInputActionValue& Value)
{
	const float DeltaTime = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
	const bool bIsComplete = HandleInteractHoldProgress(DeltaTime);
	if (bIsComplete)
	{
		InteractionCompleteResult();
	}
}

void UInteractComponent::OnInteractHoldCanceled(const FInputActionValue& Value)
{
	HandleInteractHoldCanceled();
}

void UInteractComponent::Server_SyncHoldProgress_Implementation(AActor* InteractTarget, float DeltaTime)
{
	if (IInteractableInterface* Interactable = Cast<IInteractableInterface>(InteractTarget))
	{
		Interactable->OnServerHoldProgress(DeltaTime, GetOwnerCharacter());
	}
}

bool UInteractComponent::Server_SyncHoldProgress_Validate(AActor* InteractTarget, float DeltaTime)
{
	return true;
}

void UInteractComponent::OnQTEInteractStarted()
{
	HandleInteractQTEStarted();
}

void UInteractComponent::OnQTEInput(const FVector2D& InputVector)
{
	EQTEDirection FinalDir = EQTEDirection::None;

	if (FMath::Abs(InputVector.X) > FMath::Abs(InputVector.Y))
	{
		FinalDir = (InputVector.X > 0) ? EQTEDirection::Right : EQTEDirection::Left;
	}
	else
	{
		FinalDir = (InputVector.Y > 0) ? EQTEDirection::Up : EQTEDirection::Down;
	}

	if (FinalDir != EQTEDirection::None)
	{
		if (ReceiveQTEInput(FinalDir))
		{
			InteractionCompleteResult();
		}
	}
}

void UInteractComponent::SetQTEInputMode(bool bIsQTE)
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter) return;
	
	if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
	{
		PC->SetIgnoreMoveInput(bIsQTE);

		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (AMyCharacter* CharacterBridge = GetOwnerCharacterBridge())
			{
				if (bIsQTE)
				{
					if (CharacterBridge->IMC_Default)
					{
						Subsystem->RemoveMappingContext(CharacterBridge->IMC_Default);
					}
					if (CharacterBridge->IMC_QTE)
					{
						Subsystem->AddMappingContext(CharacterBridge->IMC_QTE, 1);
					}

					if (CharacterBridge->GetCharacterMovement())
					{
						CharacterBridge->GetCharacterMovement()->StopMovementImmediately();
					}
				}
				else
				{
					if (CharacterBridge->IMC_QTE)
					{
						Subsystem->RemoveMappingContext(CharacterBridge->IMC_QTE);
					}
					if (CharacterBridge->IMC_Default)
					{
						Subsystem->AddMappingContext(CharacterBridge->IMC_Default, 0);
					}
				}
			}
		}
	}
}

void UInteractComponent::InteractionCompleteResult()
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter) return;

	if (!LastInteractedActor)
	{
		return;
	}

	if (IA302AnimationBridge* Anim = Cast<IA302AnimationBridge>(OwnerCharacter->GetMesh()->GetAnimInstance()))
	{
		Anim->StopStatueInteractAnimation();
		Anim->PlayInteractAnimation();
	}

	// Statue 상호작용 완료 시 1인칭 복귀
	if (OwnerCharacter->IsLocallyControlled() && Cast<AStatueInteractable>(LastInteractedActor))
	{
		if (AMyCharacter* MyChar = Cast<AMyCharacter>(OwnerCharacter))
		{
			MyChar->SetCameraViewMode(EA302CameraViewMode::FirstPersonChest);
			UE_LOG(LogTemp, Warning, TEXT("[InteractComponent] Statue 상호작용 완료 - 1인칭 카메라 복귀"));
		}
	}

	ABaseInteractable* Interactable = Cast<ABaseInteractable>(LastInteractedActor);
	if (!IsValid(Interactable))
	{
		return;
	}

	Interactable->OnInteractionSuccess(OwnerCharacter);

	if (UCharacterRewardComponent* RewardComp = OwnerCharacter->FindComponentByClass<UCharacterRewardComponent>())
	{
		if (OwnerCharacter->HasAuthority())
		{
			RewardComp->ResolveInteractionRewardOnServer(Interactable);
		}
		else
		{
			RewardComp->Server_RequestInteractionReward(Interactable);
		}
	}
}

void UInteractComponent::HandleInteractQTEStarted()
{
	if (!LastInteractableActor) return;
	
	LastInteractedActor = nullptr;
	
	if (IInteractableInterface* Interactable = Cast<IInteractableInterface>(LastInteractableActor))
	{
		if (Interactable->GetInteractType() == EInteractType::QTE)
		{
			// 1. 초기화
			TargetQTEKeys.Empty();
			CurrentQTEIndex = 0;
			bIsQTEActive = true;

			// 2. 입력 요구 개수 랜덤으로 결정
			int32 QTECount = FMath::RandRange(6, 10);

			// 3. 랜덤 방향 채우기
			for (int32 i = 0; i < QTECount; ++i)
			{
				// Up, Down, Left, Right 중 하나 랜덤 선택
				EQTEDirection RandomDir = static_cast<EQTEDirection>(FMath::RandRange(0, 3));
				TargetQTEKeys.Add(RandomDir);
			}
			OnQTEStarted.Broadcast(TargetQTEKeys);
			
			// 4. 로그 출력 (디버깅용)
			UE_LOG(LogTemp, Warning, TEXT("[QTE] %d개의 화살표 생성됨!"), QTECount);
            
			// TODO: UI 위젯을 띄우고 생성된 TargetQTEKeys 전달하기
			SetQTEInputMode(true); // IMC 교체 및 이동 제한 호출
		}
	}
}

bool UInteractComponent::ReceiveQTEInput(EQTEDirection InputDir)
{
	if (!bIsQTEActive || TargetQTEKeys.Num() == 0) return false;

	if (TargetQTEKeys[CurrentQTEIndex] == InputDir)
	{
		CurrentQTEIndex++;
		OnQTEProgressUpdated.Broadcast(CurrentQTEIndex);

		if (CurrentQTEIndex >= TargetQTEKeys.Num())
		{
			OnQTESuccess();
			return true; // 🚩 성공적으로 끝났음을 알림!
		}
	}
	else
	{
		OnQTEFailure();
	}
	return false;
}

void UInteractComponent::OnQTESuccess()
{
	UE_LOG(LogTemp, Warning, TEXT("[QTE] 모든 입력 성공!"));
	bIsQTEActive = false;
	OnQTEEnded.Broadcast(true);

	// 실제 상호작용 실행 (아이템 획득 등)
	if (IInteractableInterface* Interactable = Cast<IInteractableInterface>(LastInteractableActor))
	{
		Interactable->Interact(GetOwnerCharacter());
		LastInteractedActor = LastInteractableActor;
	}
    
	SetQTEInputMode(false); // 조작권 복구
}

void UInteractComponent::OnQTEFailure()
{
	CurrentQTEIndex = 0;
	bIsQTEActive = false;
	OnQTEEnded.Broadcast(false);
    
	SetQTEInputMode(false); // 조작권 복구
}
