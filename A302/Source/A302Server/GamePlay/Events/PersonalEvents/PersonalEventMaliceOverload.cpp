#include "GamePlay/Events/PersonalEvents/PersonalEventMaliceOverload.h"
#include "GameData/Events/PersonalEvents/Malice/PersonalEventMaliceOverloadDefinition.h"

#include "Character/Components/MaliceComponent.h"
#include "GameFramework/PlayerState.h"
#include "Character/MyCharacter.h"
#include "Character/MyPlayerController.h"
#include "Character/Components/PlayerEventComponent.h"
#include "Engine/World.h"
#include "Room/RoomScopeRules.h"

namespace
{
	constexpr float MaliceOverloadNotificationRadius = 1500.0f;
}

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

	UMaliceComponent* MaliceComponent = InstigatorCharacter->FindComponentByClass<UMaliceComponent>();
	const int32 CurrentMalice = MaliceComponent ? FMath::Max(0, MaliceComponent->MaliceCount) : 0;

	FString Description = TEXT("날카로운 모서리에 손을 베였습니다. 악의가 1 오릅니다.");
	if (CurrentMalice >= 2)
	{
		Description += TEXT("\n\n차오르는 악의가 주변에 흩뿌려집니다. 주변의 누군가가 이를 느꼈을지도 모르겠습니다.");
	}

	PlayerController->Client_ShowTitleCard(
		FText::FromString(TEXT("끓어오르는 악의")),
		FText::FromString(Description),
		5.0f
	);

	OnEventResolved(InstigatorCharacter, 0);
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
		if (UWorld* World = InstigatorCharacter->GetWorld())
		{
			const FString SourceRoomCode = A302RoomScope::ResolvePlayerRoomCode(InstigatorCharacter->GetPlayerState());
			const FVector SourceLocation = InstigatorCharacter->GetActorLocation();
			const float RadiusSquared = FMath::Square(MaliceOverloadNotificationRadius);
			const FText NotificationMessage = FText::FromString(TEXT("악인의 짙은 악의가 느껴집니다."));

			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				AMyPlayerController* TargetController = Cast<AMyPlayerController>(It->Get());
				if (!TargetController)
				{
					continue;
				}

				APawn* TargetPawn = TargetController->GetPawn();
				if (!TargetPawn || TargetPawn == InstigatorCharacter)
				{
					continue;
				}

				if (!SourceRoomCode.IsEmpty())
				{
					const FString TargetRoomCode = A302RoomScope::ResolvePlayerRoomCode(TargetController->PlayerState);
					if (!A302RoomScope::MatchesRoomCodeStrict(SourceRoomCode, TargetRoomCode))
					{
						continue;
					}
				}

				if (FVector::DistSquared(SourceLocation, TargetPawn->GetActorLocation()) > RadiusSquared)
				{
					continue;
				}

				TargetController->Client_ShowNotificationMessage(NotificationMessage);
			}
		}
	}

	OnEventResolved_Implementation(InstigatorCharacter, true);
}
