#include "GamePlay/Events/PersonalEvents/PersonalEventAnothersPortrait.h"

#include "Character/Components/ItemManagerComponent.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "GameData/Events/PersonalEvents/PersonalEventAnothersPortraitDefinition.h"
#include "GameData/Items/ItemDefinition.h"
#include "GamePlay/Items/BaseItem.h"

void UPersonalEventAnothersPortrait::ExecuteEvent_Implementation(ACharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter)
	{
		return;
	}

	AMyPlayerController* PlayerController = Cast<AMyPlayerController>(InstigatorCharacter->GetController());
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AnothersPortrait] PlayerController is missing."));
		return;
	}

	UPlayerEventComponent* EventComp = PlayerController->GetPlayerEventComponent();
	if (!EventComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AnothersPortrait] PlayerEventComponent is missing."));
		return;
	}

	TArray<FText> Choices;
	Choices.Add(FText::FromString(TEXT("확인")));

	EventComp->SetActivePersonalEvent(this);
	EventComp->ShowPersonalEvent(
		EventID,
		FText::FromString(TEXT("타인의 초상")),
		FText::FromString(TEXT("불길하게 생긴 거울을 발견했습니다. 그 속에 비친 것은 자신의 모습이 아닌, 다른 사람의 일그러진 욕망입니다. 거울에 금이 가는 순간, 서로가 품어온 업보의 무게가 거울 너머로 쏟아져 들어가 서로의 자리를 대신할 것입니다.")),
		Choices
	);
}

void UPersonalEventAnothersPortrait::OnEventResolved_Implementation(ACharacter* InstigatorCharacter, int32 ChoiceIndex)
{
	(void)ChoiceIndex;

	if (!InstigatorCharacter)
	{
		return;
	}

	const UPersonalEventAnothersPortraitDefinition* EventDefinition =
		Cast<UPersonalEventAnothersPortraitDefinition>(GetRewardDefinition());
	if (!EventDefinition || !EventDefinition->OminousMirrorDefinition)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AnothersPortrait] OminousMirrorDefinition is missing."));
		return;
	}

	UItemManagerComponent* ItemManager = InstigatorCharacter->FindComponentByClass<UItemManagerComponent>();
	if (!ItemManager)
	{
		return;
	}

	int32 AddedSlotIndex = INDEX_NONE;
	if (!ItemManager->TryAddItemToFirstEmptySlot(EventDefinition->OminousMirrorDefinition, 1, AddedSlotIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[AnothersPortrait] Failed to grant OminousMirror."));
		return;
	}

	if (UClass* LogicClass = EventDefinition->OminousMirrorDefinition->ResolveRewardLogicClass())
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
