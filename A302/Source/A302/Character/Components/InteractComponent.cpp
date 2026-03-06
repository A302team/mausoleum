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

	// Moved from old MyCharacter::BeginPlay (interaction widget create/show-hide section).
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

			const FString DebugMsg = FString::Printf(TEXT("Interactable: %s"), *Interactable->GetInteractText());
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
			const ESlateVisibility NewVisibility =
				CurrentHitActor ? ESlateVisibility::Visible : ESlateVisibility::Hidden;
			InteractionWidgetInstance->SetVisibility(NewVisibility);
		}

		LastInteractableActor = CurrentHitActor;
	}
}

void UInteractComponent::ToggleHighlight(AActor* TargetActor, bool bIsOn) const
{
	if (!TargetActor)
	{
		return;
	}

	// Moved from old MyCharacter::ToggleHighlight.
	TArray<UMeshComponent*> MeshComps;
	TargetActor->GetComponents<UMeshComponent>(MeshComps);

	for (UMeshComponent* MeshComp : MeshComps)
	{
		MeshComp->SetRenderCustomDepth(bIsOn);
	}
}

void UInteractComponent::HandleInteractHoldProgress(float DeltaTime)
{
	if (!LastInteractableActor) return;

	if (IInteractableInterface* Interactable = Cast<IInteractableInterface>(LastInteractableActor))
	{
		if (Interactable->GetInteractType() == EInteractType::Hold)
		{
			InteractionProgressRatio += (DeltaTime / MaxHoldTime);
			InteractionProgressRatio = FMath::Clamp(InteractionProgressRatio, 0.0f, 1.0f);
		}
	}
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
			InteractionProgressRatio = 0.0f;
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

	if (IInteractableInterface* Interactable = Cast<IInteractableInterface>(LastInteractableActor))
	{
		if (Interactable->GetInteractType() == EInteractType::QTE)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Interaction] QTE 타입 감지"));
          
			// 기존 HandleInteractInput처럼 즉시 상호작용 실행(이후 QTE 기능으로 변경 예정)
			Interactable->Interact(GetOwnerCharacter());
			LastInteractedActor = LastInteractableActor;
		}
	}
}
