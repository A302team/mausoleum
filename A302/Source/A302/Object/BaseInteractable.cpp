#include "Object/BaseInteractable.h"
#include "Math/UnrealMathUtility.h"
#include "Character/MyCharacter.h"
#include "GameData/ItemDefinition.h"
#include "GamePlay/Events/BaseEvent.h"
#include "Character/Components/QuickSlotComponent.h"

ABaseInteractable::ABaseInteractable()
{
	PrimaryActorTick.bCanEverTick = false;
    
	CurrentInteractType = EInteractType::Hold;
    
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	RootComponent = Mesh;
    
	int32 RandomIndex = FMath::RandRange(0, static_cast<int32>(EInteractType::MAX) - 1);
	CurrentInteractType = static_cast<EInteractType>(RandomIndex);
}

void ABaseInteractable::Interact(AMyCharacter* PlayerCharacter)
{
	// 상호작용한 캐릭터를 로그로 출력
	if (PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Interaction] %s interacted with %s!"), *PlayerCharacter->GetName(), *GetName());
	}
}

void ABaseInteractable::OnInteractionSuccess(AMyCharacter* PlayerCharacter)
{
	if (!PlayerCharacter) return;

	// 선택된 보상 타입에 따라 분기 처리
	switch (RewardType)
	{
	case ERewardType::Event:
		{
			if (RewardEventClass)
			{
				UBaseEvent* NewEvent = NewObject<UBaseEvent>(this, RewardEventClass);
				if (NewEvent)
				{
					NewEvent->ExecuteEvent(PlayerCharacter);
					UE_LOG(LogTemp, Warning, TEXT("[Interaction] Event Triggered: %s"), *RewardEventClass->GetName());
				}
				Destroy();
			}
			break;
		}
	case ERewardType::Item:
		{
			if (RewardItem)
			{
				// UQuickSlotComponent* QuickSlot = PlayerCharacter->FindComponentByClass<UQuickSlotComponent>();
				// if (QuickSlot) QuickSlot->TryAddItemByDefinition(RewardItem);
                
				UE_LOG(LogTemp, Warning, TEXT("[Interaction] Item Rewarded: %s"), *RewardItem->GetName());
			}
			break;
		}
	case ERewardType::None:
	default:
		{
			// 예외 처리
			UE_LOG(LogTemp, Warning, TEXT("[Interaction] Interacted with no specific reward."));
			break;
		}
	}
}

FString ABaseInteractable::GetInteractText()
{
	// 보상 타입에 따라 텍스트를 다르게 출력
	if (RewardType == ERewardType::Event)
	{
		return FString::Printf(TEXT("수상한 물체 (Interact)"));
	}
	else if (RewardType == ERewardType::Item && RewardItem && !RewardItem->DisplayName.IsEmpty())
	{
		return FString::Printf(TEXT("%s (Interact)"), *RewardItem->DisplayName.ToString());
	}

	return FString::Printf(TEXT("%s (Interact)"), *GetName());
}

EInteractType ABaseInteractable::GetInteractType()
{
	return CurrentInteractType;
}
