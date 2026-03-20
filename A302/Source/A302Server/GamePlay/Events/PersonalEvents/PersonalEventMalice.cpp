#include "GamePlay/Events/PersonalEvents/PersonalEventMalice.h"

#include "Character/Components/MaliceComponent.h"
#include "Character/Components/PlayerEventComponent.h"
#include "Character/MyPlayerController.h"
#include "GameData/Events/PersonalEvents/Malice/PersonalEventMaliceDefinition.h"
#include "GameData/RewardDefinition.h"

void UPersonalEventMalice::ExecuteEvent_Implementation(ACharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter)
	{
		return;
	}

	AMyPlayerController* PlayerController = Cast<AMyPlayerController>(InstigatorCharacter->GetController());
	if (!PlayerController)
	{
		return;
	}

	UPlayerEventComponent* EventComp = PlayerController->GetPlayerEventComponent();
	if (!EventComp)
	{
		return;
	}

	TArray<FText> Choices;
	Choices.Add(FText::FromString(TEXT("확인")));

	EventComp->SetActivePersonalEvent(this);
	EventComp->ShowPersonalEvent(
		EventID,
		FText::FromString(TEXT("불의의 사고")),
		FText::FromString(TEXT("조사 도중 모서리에 팔꿈치를 부딪쳤습니다! 짜증이 치밀어 오릅니다...")),
		Choices
	);
}

void UPersonalEventMalice::OnEventResolved(ACharacter* InstigatorCharacter, int32 ChoiceIndex)
{
	(void)ChoiceIndex;

	if (!InstigatorCharacter)
	{
		return;
	}

	const UPersonalEventMaliceDefinition* EventDef =
		Cast<UPersonalEventMaliceDefinition>(const_cast<URewardDefinition*>(GetRewardDefinition()));
	const int32 MaliceAmount = EventDef ? FMath::Max(1, EventDef->Payload.MaliceAmount) : 1;

	if (UMaliceComponent* MaliceComponent = InstigatorCharacter->FindComponentByClass<UMaliceComponent>())
	{
		MaliceComponent->AddMalice(MaliceAmount);
		UE_LOG(LogTemp, Log, TEXT("[PersonalEventMalice] Added malice: +%d"), MaliceAmount);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PersonalEventMalice] MaliceComponent missing."));
	}

	OnEventResolved_Implementation(InstigatorCharacter, true);
}
