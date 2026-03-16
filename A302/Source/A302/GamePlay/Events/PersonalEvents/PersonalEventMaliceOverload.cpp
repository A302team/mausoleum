#include "GamePlay/Events/PersonalEvents/PersonalEventMaliceOverload.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "Character/Components/MaliceComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"

void UPersonalEventMaliceOverload::ExecuteEvent_Implementation(AMyCharacter* InstigatorCharacter)
{
    if (!InstigatorCharacter) return;
    AMyPlayerController* PC = Cast<AMyPlayerController>(InstigatorCharacter->GetController());
    if (!PC) return;

    PC->ActivePersonalEvent = this;
    
    UMaliceComponent* MaliceComp = InstigatorCharacter->FindComponentByClass<UMaliceComponent>();
    int32 CurrentMalice = MaliceComp ? MaliceComp->MaliceCount : 0;
    
    FString Description = TEXT("날카로운 모서리에 손을 베였습니다. 악의가 1 오릅니다.");
    
    if (CurrentMalice >= 2) 
    {
        Description += TEXT("\n\n차오르는 악의가 주변에 흩뿌려집니다. 주변의 누군가가 이를 느꼈을지도 모르겠습니다.");
    }

    // 선택지 구성 (확인 버튼 1개)
    TArray<FText> Choices;
    Choices.Add(FText::FromString(TEXT("확인")));

    PC->Client_ShowPersonalEvent(
        EventID, 
        FText::FromString(TEXT("끓어오르는 악의")), 
        FText::FromString(Description),
        Choices
    );
}

void UPersonalEventMaliceOverload::OnEventResolved_Implementation(AMyCharacter* InstigatorCharacter, int32 ChoiceIndex)
{
    
    // ChoiceIndex 0: '확인'을 눌렀을 때만 동작
    if (ChoiceIndex != 0 || !InstigatorCharacter) return;

    UMaliceComponent* MaliceComp = InstigatorCharacter->FindComponentByClass<UMaliceComponent>();
    if (!MaliceComp) return;

    // 1. 공통 로직: 악의 1 증가
    MaliceComp->AddMalice(1);

    // 2. 조건부 광역 로직: 악인일 때 메시지 전파
    if (MaliceComp->MaliceCount >= 3)
    {
        const float DetectionRadius = 1500.0f; // 감지 반경
        FVector MyLocation = InstigatorCharacter->GetActorLocation();

        // 월드 내의 모든 플레이어 컨트롤러를 순회
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            AMyPlayerController* TargetPC = Cast<AMyPlayerController>(It->Get());
            if (!TargetPC || TargetPC == InstigatorCharacter->GetController()) continue;

            AMyCharacter* TargetChar = Cast<AMyCharacter>(TargetPC->GetPawn());
            if (TargetChar)
            {
                float Distance = FVector::Dist(MyLocation, TargetChar->GetActorLocation());
                if (Distance <= DetectionRadius)
                {
                    TargetPC->Client_ReceiveSystemMessage(TEXT("악인의 짙은 악의가 느껴집니다"));
                }
            }
        }
    }
}