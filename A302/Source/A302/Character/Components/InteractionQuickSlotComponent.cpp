#include "Character/Components/InteractionQuickSlotComponent.h"

#include "Blueprint/UserWidget.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "Components/MeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "GameData/ItemDefinition.h"
#include "Interface/InteractableInterface.h"
#include "Object/BaseInteractable.h"

namespace
{
	constexpr int32 MaxQuickSlotCount = 5;

	void LogAndScreen(const FString& Message, const FColor& Color = FColor::Yellow, const float Duration = 3.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, Duration, Color, Message);
		}
	}
}

UInteractionQuickSlotComponent::UInteractionQuickSlotComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UInteractionQuickSlotComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeQuickSlots();

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

void UInteractionQuickSlotComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction
)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	CheckForInteractables();
}

void UInteractionQuickSlotComponent::HandleInteractInput()
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		return;
	}

	FVector Start = OwnerCharacter->GetPawnViewLocation();
	FVector ForwardVector = OwnerCharacter->GetViewRotation().Vector();
	FVector End = Start + (ForwardVector * InteractionDistance);

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(OwnerCharacter);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, Params))
	{
		if (Cast<IInteractableInterface>(HitResult.GetActor()))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Interaction] F pressed. Target: %s"), *HitResult.GetActor()->GetName());
			HandleInteractOnActor(HitResult.GetActor());
		}
	}
}

AMyCharacter* UInteractionQuickSlotComponent::GetOwnerCharacter() const
{
	return Cast<AMyCharacter>(GetOwner());
}

void UInteractionQuickSlotComponent::CheckForInteractables()
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || !OwnerCharacter->IsLocallyControlled())
	{
		return;
	}

	FVector Start = OwnerCharacter->GetPawnViewLocation();
	FVector ForwardVector = OwnerCharacter->GetViewRotation().Vector();
	FVector End = Start + (ForwardVector * InteractionDistance);

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

			const FString DebugMsg = FString::Printf(
				TEXT("Interactable: %s"),
				*Interactable->GetInteractText()
			);

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

void UInteractionQuickSlotComponent::ToggleHighlight(AActor* TargetActor, bool bIsOn) const
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
	}
}

void UInteractionQuickSlotComponent::HandleInteractOnActor(AActor* TargetActor)
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || !TargetActor)
	{
		return;
	}

	if (IInteractableInterface* Interactable = Cast<IInteractableInterface>(TargetActor))
	{
		Interactable->Interact(OwnerCharacter);

		if (TryPickupItemToQuickSlot(TargetActor))
		{
			TargetActor->Destroy();
		}
	}
}

bool UInteractionQuickSlotComponent::TryPickupItemToQuickSlot(AActor* TargetActor)
{
	UItemDefinition* ItemDefinition = nullptr;
	if (!TryGetItemDefinitionFromActor(TargetActor, ItemDefinition) || !ItemDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] Target has no ItemDefinition: %s"), *GetNameSafe(TargetActor));
		LogAndScreen(TEXT("[QuickSlot] Pickup failed: ItemDefinition is missing on actor."), FColor::Orange, 2.0f);
		return false;
	}

	const int32 EmptySlotIndex = FindEmptyQuickSlotIndex();
	if (EmptySlotIndex == INDEX_NONE)
	{
		LogAndScreen(TEXT("[QuickSlot] Slot is full (Max 5)."), FColor::Orange, 2.0f);
		return false;
	}

	QuickSlotItems[EmptySlotIndex] = ItemDefinition;
	UpdateQuickSlotNameUI(EmptySlotIndex, ItemDefinition);

	const FString PickedName = ItemDefinition->DisplayName.IsEmpty()
		? ItemDefinition->ItemId.ToString()
		: ItemDefinition->DisplayName.ToString();

	LogAndScreen(
		FString::Printf(TEXT("[QuickSlot] Picked '%s' -> Slot %d"), *PickedName, EmptySlotIndex + 1),
		FColor::Green,
		2.0f
	);

	return true;
}

bool UInteractionQuickSlotComponent::TryGetItemDefinitionFromActor(
	AActor* TargetActor,
	UItemDefinition*& OutItemDefinition
) const
{
	OutItemDefinition = nullptr;

	if (ABaseInteractable* InteractableActor = Cast<ABaseInteractable>(TargetActor))
	{
		OutItemDefinition = InteractableActor->GetItemDefinition();
	}

	return OutItemDefinition != nullptr;
}

int32 UInteractionQuickSlotComponent::FindEmptyQuickSlotIndex() const
{
	for (int32 SlotIndex = 0; SlotIndex < QuickSlotItems.Num(); ++SlotIndex)
	{
		if (QuickSlotItems[SlotIndex] == nullptr)
		{
			return SlotIndex;
		}
	}

	return INDEX_NONE;
}

void UInteractionQuickSlotComponent::InitializeQuickSlots()
{
	QuickSlotItems.Init(nullptr, MaxQuickSlotCount);
}

void UInteractionQuickSlotComponent::UpdateQuickSlotNameUI(int32 SlotIndex, const UItemDefinition* ItemDefinition) const
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || !ItemDefinition)
	{
		return;
	}

	AController* RawController = OwnerCharacter->GetController();
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("[QuickSlot] Update UI try. Controller=%s Class=%s Slot=%d Item=%s"),
		*GetNameSafe(RawController),
		*GetNameSafe(RawController ? RawController->GetClass() : nullptr),
		SlotIndex + 1,
		*ItemDefinition->ItemId.ToString()
	);

	AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(RawController);
	if (!MyPlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] MyPlayerController is null."));
		LogAndScreen(TEXT("[QuickSlot] UI skipped: PlayerController is not AMyPlayerController."), FColor::Orange, 2.0f);
		return;
	}

	const FText ItemName = ItemDefinition->DisplayName.IsEmpty()
		? FText::FromName(ItemDefinition->ItemId)
		: ItemDefinition->DisplayName;

	if (!MyPlayerController->UpdateQuickSlotItemVisual(SlotIndex, ItemName, ItemDefinition->Icon))
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] Failed to refresh UI for slot %d."), SlotIndex + 1);
		LogAndScreen(TEXT("[QuickSlot] UI update failed. Check QuickSlot widget names."), FColor::Orange, 2.0f);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] UI updated for slot %d."), SlotIndex + 1);
	}
}
