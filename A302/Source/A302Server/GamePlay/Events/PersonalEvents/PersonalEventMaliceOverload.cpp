#include "GamePlay/Events/PersonalEvents/PersonalEventMaliceOverload.h"
#include "GameData/Events/PersonalEvents/Malice/PersonalEventMaliceOverloadDefinition.h"

#include "Character/Components/MaliceComponent.h"
#include "GameFramework/PlayerState.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "Character/Components/PlayerEventComponent.h"

void UPersonalEventMaliceOverload::ExecuteEvent_Implementation(ACharacter* InstigatorCharacter)
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

	EventComp->SetActivePersonalEvent(this);

	UMaliceComponent* MaliceComponent = InstigatorCharacter->FindComponentByClass<UMaliceComponent>();
	const int32 CurrentMalice = MaliceComponent ? FMath::Max(0, MaliceComponent->MaliceCount) : 0;

	FString Description = TEXT("날카로운 모서리에 손을 베였습니다. 악의가 1 오릅니다.");
	if (CurrentMalice >= 2)
	{
		Description += TEXT("\n\n차오르는 악의가 주변에 흩뿌려집니다. 주변의 누군가가 이를 느꼈을지도 모르겠습니다.");
	}

	TArray<FText> Choices;
	Choices.Add(FText::FromString(TEXT("확인")));

	EventComp->ShowPersonalEvent(
		EventID,
		FText::FromString(TEXT("끓어오르는 악의")),
		FText::FromString(Description),
		Choices
	);
}

void UPersonalEventMaliceOverload::OnEventResolved(ACharacter* InstigatorCharacter, int32 ChoiceIndex)
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

		if (AMyCharacter* CharacterBridge = Cast<AMyCharacter>(InstigatorCharacter))
		{
			CharacterBridge->BroadcastPublicMaliceAnnouncement(PlayerName, CurrentMalice);
		}
		else if (AMyPlayerController* ClientEventBridge = Cast<AMyPlayerController>(InstigatorCharacter->GetController()))
		{
			ClientEventBridge->ShowPublicMaliceAnnouncement(PlayerName, CurrentMalice);
		}
	}

	OnEventResolved_Implementation(InstigatorCharacter, true);
}
