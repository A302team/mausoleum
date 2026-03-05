#include "Character/Components/QuickSlotComponent.h"

#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "GameData/ItemDefinition.h"
#include "GameFramework/Controller.h"

void UQuickSlotComponent::UpdateQuickSlotSelectionUI() const
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		return;
	}

	AController* RawController = OwnerCharacter->GetController();
	AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(RawController);
	if (!MyPlayerController)
	{
		return;
	}

	MyPlayerController->UpdateQuickSlotSelectionVisual(SelectedSlotIndex);
}

void UQuickSlotComponent::UpdateQuickSlotNameUI(int32 SlotIndex, const UItemDefinition* ItemDefinition) const
{
	AMyCharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter)
	{
		return;
	}

	AController* RawController = OwnerCharacter->GetController();
	AMyPlayerController* MyPlayerController = Cast<AMyPlayerController>(RawController);
	if (!MyPlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] UI skipped: PlayerController is not AMyPlayerController."));
		return;
	}

	if (!ItemDefinition)
	{
		MyPlayerController->UpdateQuickSlotItemVisual(SlotIndex, FText::GetEmpty(), nullptr);
		return;
	}

	const FText ItemName = ItemDefinition->DisplayName.IsEmpty()
		? FText::FromName(ItemDefinition->ItemId)
		: ItemDefinition->DisplayName;

	if (!MyPlayerController->UpdateQuickSlotItemVisual(SlotIndex, ItemName, ItemDefinition->Icon))
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuickSlot] Failed to refresh UI for slot %d."), SlotIndex + 1);
		LogAndScreenQuickSlotMessage(TEXT("[QuickSlot] UI update failed. Check QuickSlot widget names."), FColor::Orange, 2.0f);
	}
}
