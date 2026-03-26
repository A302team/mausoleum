#include "GamePlay/Events/PersonalEvents/PersonalEventAnothersPortrait.h"

#include "Character/Components/Inventory/ItemManagerComponent.h"
#include "Character/Components/Interaction/CharacterRewardComponent.h"
#include "Character/Components/PlayerEventComponent.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "GameData/Events/PersonalEvents/Malice/PersonalEventAnothersPortraitDefinition.h"
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

	if (AMyPlayerController* PlayerControllerX = Cast<AMyPlayerController>(InstigatorCharacter->GetController()))
	{
		PlayerControllerX->Client_ShowTitleCard(
			FText::FromString(TEXT("타인의 초상")),
			FText::FromString(TEXT("불길하게 생긴 거울을 발견했습니다. 그 속에 비친 것은 자신의 모습이 아닌, 다른 사람의 일그러진 욕망입니다. 거울에 금이 가는 순간, 서로가 품어온 업보의 무게가 거울 너머로 쏟아져 들어가 서로의 자리를 대신할 것입니다.")),
			5.0f
		);
	}

	OnEventResolved(InstigatorCharacter, 0);
}

void UPersonalEventAnothersPortrait::OnEventResolved(ACharacter* InstigatorCharacter, int32 ChoiceIndex)
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

	if (UCharacterRewardComponent* RewardComponent = InstigatorCharacter->FindComponentByClass<UCharacterRewardComponent>())
	{
		const bool bNeedsClientMirrorGrant =
			!InstigatorCharacter->IsLocallyControlled() &&
			Cast<AMyPlayerController>(InstigatorCharacter->GetController()) != nullptr;

		if (bNeedsClientMirrorGrant)
		{
			RewardComponent->Client_GrantInteractionReward(EventDefinition->OminousMirrorDefinition);
		}
	}
}
