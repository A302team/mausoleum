#include "GamePlay/Events/PersonalEvents/PersonalEventDevilsEye.h"

#include "Character/Components/Inventory/ItemManagerComponent.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "Character/Components/PlayerEventComponent.h"
#include "GameData/Events/PersonalEvents/Status/PersonalEventDevilsEyeDefinition.h"
#include "GameData/Items/ItemDefinition.h"
#include "GamePlay/Items/BaseItem.h"
#include "Character/Components/PlayerEventComponent.h"

namespace
{
	const FText DevilsEyeTitle = FText::FromString(TEXT("악마의 눈동자"));
	const FText DevilsEyeDescription = FText::FromString(
		TEXT("눈동자의 형상을 한 붉은 수정구를 얻었습니다. 수정구를 사용하여 타인의 내면을 들여다볼 수 있을 것 같습니다.")
	);
	const FText DevilsEyeConfirmChoice = FText::FromString(TEXT("확인"));
}

void UPersonalEventDevilsEye::ExecuteEvent_Implementation(ACharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter)
	{
		return;
	}

	AMyPlayerController* PlayerController = Cast<AMyPlayerController>(InstigatorCharacter->GetController());
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DevilsEye] PlayerController is missing."));
		return;
	}

	PlayerController->Client_ShowTitleCard(
		DevilsEyeTitle,
		DevilsEyeDescription,
		5.0f
	);

	OnEventResolved(InstigatorCharacter, 0);
}

void UPersonalEventDevilsEye::OnEventResolved(ACharacter* InstigatorCharacter, int32 ChoiceIndex)
{
	(void)ChoiceIndex;

	if (!InstigatorCharacter)
	{
		return;
	}

	const UPersonalEventDevilsEyeDefinition* EventDefinition = Cast<UPersonalEventDevilsEyeDefinition>(GetRewardDefinition());
	if (!EventDefinition || !EventDefinition->CrimsonQuartzDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DevilsEye] CrimsonQuartzDefinition is missing."));
		return;
	}

	UItemManagerComponent* ItemManager = InstigatorCharacter->FindComponentByClass<UItemManagerComponent>();
	if (!ItemManager)
	{
		return;
	}

	int32 AddedSlotIndex = INDEX_NONE;
	if (!ItemManager->TryAddItemToFirstEmptySlot(EventDefinition->CrimsonQuartzDefinition, 1, AddedSlotIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[DevilsEye] Failed to grant CrimsonQuartz."));
		return;
	}

	if (UClass* LogicClass = EventDefinition->CrimsonQuartzDefinition->ResolveRewardLogicClass())
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
