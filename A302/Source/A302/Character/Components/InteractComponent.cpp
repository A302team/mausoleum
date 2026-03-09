#include "Character/Components/InteractComponent.h"
#include "Blueprint/UserWidget.h"
#include "Character/MyCharacter.h"
#include "Components/MeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Interface/InteractableInterface.h"

UInteractComponent::UInteractComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UInteractComponent::BeginPlay()
{
	Super::BeginPlay();

	if (InteractionWidgetClass)
	{
		InteractionWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), InteractionWidgetClass);
		if (InteractionWidgetInstance)
		{
			InteractionWidgetInstance->AddToViewport(10);
			InteractionWidgetInstance->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (CrosshairWidgetClass)
	{
		CrosshairWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), CrosshairWidgetClass);
		if (CrosshairWidgetInstance)
		{
			CrosshairWidgetInstance->AddToViewport(20);
		}
	}
    
	if (QTEWidgetClass)
	{
		QTEWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), QTEWidgetClass);
		if (QTEWidgetInstance)
		{
			QTEWidgetInstance->AddToViewport(50);
			QTEWidgetInstance->SetVisibility(ESlateVisibility::Collapsed);
			UE_LOG(LogTemp, Warning, TEXT("[C++] QTE 위젯 생성 및 뷰포트 추가 완료!"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[C++] QTE 위젯 생성 실패!"));
		}
	}
}

void UInteractComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	CheckForInteractables();
}

AMyCharacter* UInteractComponent::GetOwnerCharacter() const
{
	return Cast<AMyCharacter>(GetOwner());
}

void UInteractComponent::CheckForInteractables()
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled())
	{
		return;
	}
    
	const FVector Start = OwnerCharacter->GetPawnViewLocation();
	const FVector ForwardVector = OwnerCharacter->GetViewRotation().Vector();
	const FVector End = Start + (ForwardVector * InteractionDistance);

	DrawDebugLine(
		GetWorld(),
		Start,
		End,
		FColor::Red,
		false,
		-1.0f,
		0,
		0.0f
	);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerCharacter);

	AActor* CurrentHitActor = nullptr;

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
	{
		if (IInteractableInterface* Interactable = Cast<IInteractableInterface>(HitResult.GetActor()))
		{
			CurrentHitActor = HitResult.GetActor();

			FString TypeStr = (Interactable->GetInteractType() == EInteractType::Hold) ? TEXT("Hold") : TEXT("QTE");
			const FString DebugMsg = FString::Printf(TEXT("[%s] Interactable: %s"), *TypeStr, *Interactable->GetInteractText());
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(0, 0.1f, FColor::Cyan, DebugMsg);
			}
		}
	}

	if (CurrentHitActor != LastInteractableActor)
	{
		InteractionProgressRatio = 0.0f;
       
		if (LastInteractableActor)
		{
			ToggleHighlight(LastInteractableActor, false);
		}

		if (CurrentHitActor)
		{
			ToggleHighlight(CurrentHitActor, true);
		}

		if (InteractionWidgetInstance)
		{
			const ESlateVisibility NewVisibility = CurrentHitActor ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
			InteractionWidgetInstance->SetVisibility(NewVisibility);
		}

		LastInteractableActor = CurrentHitActor;
	}
}

void UInteractComponent::ToggleHighlight(AActor* TargetActor, bool bIsOn) const
{
	if (!TargetActor) return;

	TArray<UMeshComponent*> MeshComps;
	TargetActor->GetComponents<UMeshComponent>(MeshComps);

	for (UMeshComponent* MeshComp : MeshComps)
	{
		MeshComp->SetRenderCustomDepth(bIsOn);
	}
}

bool UInteractComponent::HandleInteractHoldProgress(float DeltaTime)
{
	if (!LastInteractableActor) return false;

	if (IInteractableInterface* Interactable = Cast<IInteractableInterface>(LastInteractableActor))
	{
		if (Interactable->GetInteractType() == EInteractType::Hold)
		{
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

void UInteractComponent::HandleInteractHoldComplete()
{
	LastInteractedActor = nullptr;
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || !LastInteractableActor) return;

	if (IInteractableInterface* Interactable = Cast<IInteractableInterface>(LastInteractableActor))
	{
		if (Interactable->GetInteractType() == EInteractType::Hold)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Interaction] Hold 상호작용 성공!"));
			Interactable->Interact(OwnerCharacter);
			LastInteractedActor = LastInteractableActor;
		}
	}
}

void UInteractComponent::HandleInteractHoldCanceled()
{
	InteractionProgressRatio = 0.0f;
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
			if (AMyCharacter* Owner = GetOwnerCharacter())
			{
				Owner->SetQTEInputMode(true); // IMC 교체 및 이동 제한 호출
			}
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
    
	if (AMyCharacter* Owner = GetOwnerCharacter())
	{
		Owner->SetQTEInputMode(false); // 조작권 복구
	}
}

void UInteractComponent::OnQTEFailure()
{
	CurrentQTEIndex = 0;
	bIsQTEActive = false;
	OnQTEEnded.Broadcast(false);
    
	if (AMyCharacter* Owner = GetOwnerCharacter())
	{
		Owner->SetQTEInputMode(false); // 조작권 복구
	}
}
