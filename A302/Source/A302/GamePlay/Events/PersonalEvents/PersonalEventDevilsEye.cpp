#include "GamePlay/Events/PersonalEvents/PersonalEventDevilsEye.h"

#include "Character/Components/ItemManagerComponent.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "GameData/Events/PersonalEvents/PersonalEventDevilsEyeDefinition.h"
#include "GameData/Items/ItemDefinition.h"
#include "GamePlay/Items/BaseItem.h"

void UPersonalEventDevilsEye::ExecuteEvent_Implementation(AMyCharacter* InstigatorCharacter)
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

	PlayerController->ActivePersonalEvent = this;

	TArray<FText> Choices;
	Choices.Add(FText::FromString(TEXT("확인")));

	PlayerController->Client_ShowPersonalEvent(
		EventID,
		FText::FromString(TEXT("악마의 눈동자")),
		FText::FromString(TEXT("눈동자의 형상을 한 붉은 수정구를 얻었습니다. 수정구를 사용하여 타인의 내면을 들여다볼 수 있을 것 같습니다.")),
		Choices
	);
}

void UPersonalEventDevilsEye::OnEventResolved_Implementation(AMyCharacter* InstigatorCharacter, int32 ChoiceIndex)
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
