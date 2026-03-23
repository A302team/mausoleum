#include "GamePlay/Events/PersonalEvents/PersonalEventPurifyingFlame.h"

#include "Character/Components/Inventory/ItemManagerComponent.h"
#include "Character/Components/PlayerEventComponent.h"
#include "Character/MyPlayerController.h"
#include "GameData/Events/PersonalEvents/Malice/PersonalEventPurifyingFlameDefinition.h"
#include "GameData/Items/ItemDefinition.h"
#include "GamePlay/Items/BaseItem.h"

void UPersonalEventPurifyingFlame::ExecuteEvent_Implementation(ACharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter)
	{
		return;
	}

	AMyPlayerController* PlayerController = Cast<AMyPlayerController>(InstigatorCharacter->GetController());
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PurifyingFlame] PlayerController is missing."));
		return;
	}

	if (AMyPlayerController* PlayerControllerX = Cast<AMyPlayerController>(InstigatorCharacter->GetController()))
	{
		PlayerControllerX->Client_ShowTitleCard(
			FText::FromString(TEXT("정화의 불꽃")),
			FText::FromString(TEXT("알 수 없는 문양이 새겨진 등불을 발견했습니다. 그 안의 불꽃은 미약하게 일렁이고 있으나, 내면의 어둠을 태워버릴 만큼 강렬하게 빛나고 있습니다.")),
			5.0f
		);
	}

	OnEventResolved(InstigatorCharacter, 0);
}

void UPersonalEventPurifyingFlame::OnEventResolved(ACharacter* InstigatorCharacter, int32 ChoiceIndex)
{
	(void)ChoiceIndex;

	if (!InstigatorCharacter)
	{
		return;
	}

	const UPersonalEventPurifyingFlameDefinition* EventDefinition =
		Cast<UPersonalEventPurifyingFlameDefinition>(GetRewardDefinition());
	if (!EventDefinition || !EventDefinition->SereneLanternDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PurifyingFlame] SereneLanternDefinition is missing."));
		return;
	}

	UItemManagerComponent* ItemManager = InstigatorCharacter->FindComponentByClass<UItemManagerComponent>();
	if (!ItemManager)
	{
		return;
	}

	int32 AddedSlotIndex = INDEX_NONE;
	if (!ItemManager->TryAddItemToFirstEmptySlot(EventDefinition->SereneLanternDefinition, 1, AddedSlotIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PurifyingFlame] Failed to grant SereneLantern."));
		return;
	}

	if (UClass* LogicClass = EventDefinition->SereneLanternDefinition->ResolveRewardLogicClass())
	{
		if (LogicClass->IsChildOf(UBaseItem::StaticClass()))
		{
			if (const UBaseItem* BaseItemLogic = Cast<UBaseItem>(LogicClass->GetDefaultObject()))
			{
				BaseItemLogic->OnItemAcquired(InstigatorCharacter);
			}
		}
	}
}
