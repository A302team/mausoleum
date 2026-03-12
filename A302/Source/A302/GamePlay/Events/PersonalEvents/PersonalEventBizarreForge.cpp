#include "GamePlay/Events/PersonalEvents/PersonalEventBizarreForge.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "Character/Components/MaliceComponent.h"
#include "Character/Components/ItemManagerComponent.h"
#include "GameData/Events/PersonalEvents/PersonalEventBizarreForgeDefinition.h"
#include "GamePlay/Items/BaseItem.h"
#include "GameData/Items/ItemDefinition.h"

void UPersonalEventBizarreForge::ExecuteEvent_Implementation(AMyCharacter* InstigatorCharacter)
{
    if (!InstigatorCharacter) return;
    AMyPlayerController* PC = Cast<AMyPlayerController>(InstigatorCharacter->GetController());
    if (!PC) return;

    PC->ActivePersonalEvent = this;

    UMaliceComponent* MaliceComp = InstigatorCharacter->FindComponentByClass<UMaliceComponent>();
    int32 CurrentMalice = MaliceComp ? MaliceComp->MaliceCount : 0;

    bIsMaliceZeroBranch = (CurrentMalice == 0);

    TArray<FText> Choices;

    if (bIsMaliceZeroBranch)
    {
        // 분기 A(악의 0)
        Choices.Add(FText::FromString(TEXT("확인")));
        
        PC->Client_ShowPersonalEvent(EventID, 
            FText::FromString(TEXT("기괴한 용광로")), 
            FText::FromString(TEXT("불길하게 생긴 조각상의 검은 기운이 엄습합니다.\n(악의 +1)")), 
            Choices);
    }
    else
    {
        // 분기 B(악의 1 이상)
        Choices.Add(FText::FromString(TEXT("무시하기")));
        Choices.Add(FText::FromString(TEXT("악의 서린 검 제련")));
        Choices.Add(FText::FromString(TEXT("악의 서린 방패 제련")));
        
        PC->Client_ShowPersonalEvent(EventID, 
            FText::FromString(TEXT("기괴한 용광로")), 
            FText::FromString(TEXT("불길하게 생긴 조각상이 검붉은 빛을 내뿜으며 내면의 악의와 공명합니다.\n악의를 제련할 수 있을 것 같습니다...")), 
            Choices);
    }
}

void UPersonalEventBizarreForge::OnEventResolvedMulti(AMyCharacter* InstigatorCharacter, int32 ChoiceIndex)
{
    if (!InstigatorCharacter) return;
    UMaliceComponent* MaliceComp = InstigatorCharacter->FindComponentByClass<UMaliceComponent>();

    // 악의 0 분기
    if (bIsMaliceZeroBranch)
    {
        if (MaliceComp) MaliceComp->AddMalice(1); 
        return;
    }

    // 악의 1 이상 분기
    if (ChoiceIndex == 0) return; // 0번은 무시(취소)

    if (MaliceComp && MaliceComp->MaliceCount >= 1)
    {
        const UPersonalEventBizarreForgeDefinition* EventDef = Cast<UPersonalEventBizarreForgeDefinition>(GetRewardDefinition());
        if (!EventDef) return;

        // 어떤 아이템을 줄지 결정
        UItemDefinition* ItemToGrant = nullptr;
        if (ChoiceIndex == 1)
        {
            ItemToGrant = EventDef->MaliciousSwordDefinition;
        }
        else if (ChoiceIndex == 2)
        {
            ItemToGrant = EventDef->MaliciousShieldDefinition;
        }

        // 아이템 지급 및 제련 비용 소모
        if (ItemToGrant)
        {
            if (UItemManagerComponent* ItemManager = InstigatorCharacter->FindComponentByClass<UItemManagerComponent>())
            {
                int32 AddedSlotIndex = INDEX_NONE;
                if (ItemManager->TryAddItemToFirstEmptySlot(ItemToGrant, 1, AddedSlotIndex))
                {
                    // 제련 비용: 악의 -1
                    MaliceComp->ConsumeMalice(1);
                    
                    UClass* LogicClass = ItemToGrant->ResolveRewardLogicClass();
                    if (LogicClass && LogicClass->IsChildOf(UBaseItem::StaticClass()))
                    {
                        if (const UBaseItem* BaseItemLogic = Cast<UBaseItem>(LogicClass->GetDefaultObject()))
                        {
                            BaseItemLogic->OnItemAcquired(InstigatorCharacter);
                        }
                    }
                    
                    UE_LOG(LogTemp, Warning, TEXT("[BizarreForge] %d번 선택지 제련 완료!"), ChoiceIndex);
                }
            }
        }
    }
}