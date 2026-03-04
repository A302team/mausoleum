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

void UInteractComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Moved from old MyCharacter::Tick -> CheckForInteractables call.
	CheckForInteractables();
}

void UInteractComponent::HandleInteractInput()
{
	LastInteractedActor = nullptr;

	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		return;
	}

	// Moved from old MyCharacter::OnInteract (line trace + Interactable->Interact).
	const FVector Start = OwnerCharacter->GetPawnViewLocation();
	const FVector ForwardVector = OwnerCharacter->GetViewRotation().Vector();
	const FVector End = Start + (ForwardVector * InteractionDistance);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerCharacter);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
	{
		if (IInteractableInterface* Interactable = Cast<IInteractableInterface>(HitResult.GetActor()))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Interaction] F pressed. Target: %s"), *HitResult.GetActor()->GetName());
			Interactable->Interact(OwnerCharacter);
			LastInteractedActor = HitResult.GetActor();
		}
	}
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

	// Moved from old MyCharacter::CheckForInteractables with the same control flow.
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
