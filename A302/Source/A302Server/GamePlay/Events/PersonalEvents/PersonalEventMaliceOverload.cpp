#include "GamePlay/Events/PersonalEvents/PersonalEventMaliceOverload.h"

#include "Character/Components/MaliceComponent.h"
#include "GameFramework/PlayerState.h"
#include "Interface/A302CharacterBridge.h"
#include "Interface/A302ClientEventBridge.h"

void UPersonalEventMaliceOverload::ExecuteEvent_Implementation(ACharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter)
	{
		return;
	}

	IA302ClientEventBridge* ClientEventBridge = Cast<IA302ClientEventBridge>(InstigatorCharacter->GetController());
	if (!ClientEventBridge)
	{
		return;
	}

	ClientEventBridge->SetActivePersonalEvent(this);

	UMaliceComponent* MaliceComponent = InstigatorCharacter->FindComponentByClass<UMaliceComponent>();
	const int32 CurrentMalice = MaliceComponent ? FMath::Max(0, MaliceComponent->MaliceCount) : 0;

	FString Description = TEXT("날카로운 모서리에 손을 베였습니다. 악의가 1 오릅니다.");
	if (CurrentMalice >= 2)
	{
		Description += TEXT("\n\n차오르는 악의가 주변에 흩뿌려집니다. 주변의 누군가가 이를 느꼈을지도 모르겠습니다.");
	}

	TArray<FText> Choices;
	Choices.Add(FText::FromString(TEXT("확인")));

	ClientEventBridge->ShowPersonalEvent(
		EventID,
		FText::FromString(TEXT("끓어오르는 악의")),
		FText::FromString(Description),
		Choices
	);
}

void UPersonalEventMaliceOverload::OnEventResolvedMulti(ACharacter* InstigatorCharacter, int32 ChoiceIndex)
{
	if (ChoiceIndex != 0 || !InstigatorCharacter || !InstigatorCharacter->HasAuthority())
	{
		return;
	}

	UMaliceComponent* MaliceComponent = InstigatorCharacter->FindComponentByClass<UMaliceComponent>();
	if (!MaliceComponent)
	{
		return;
	}

	MaliceComponent->AddMalice(1);

	if (MaliceComponent->MaliceCount >= 3)
	{
		const FString PlayerName = InstigatorCharacter->GetPlayerState()
			? InstigatorCharacter->GetPlayerState()->GetPlayerName()
			: GetNameSafe(InstigatorCharacter);
		const int32 CurrentMalice = FMath::Max(0, MaliceComponent->MaliceCount);

		if (IA302CharacterBridge* CharacterBridge = Cast<IA302CharacterBridge>(InstigatorCharacter))
		{
			CharacterBridge->BroadcastPublicMaliceAnnouncement(PlayerName, CurrentMalice);
		}
		else if (IA302ClientEventBridge* ClientEventBridge = Cast<IA302ClientEventBridge>(InstigatorCharacter->GetController()))
		{
			ClientEventBridge->ShowPublicMaliceAnnouncement(PlayerName, CurrentMalice);
		}
	}

	OnEventResolved(InstigatorCharacter, true);
}
