// Fill out your copyright notice in the Description page of Project Settings.

#include "GamePlay/Events/BaseEvent.h"

#include "Character/MyPlayerController.h"
#include "Engine/World.h"
#include "GameMode/A302PlayerState.h"
#include "GamePlay/Events/PersonalEvents/BasePersonalEvent.h"

void UBaseEvent::InitializeRuntimeContext(ACharacter* InstigatorCharacter)
{
	if (!InstigatorCharacter)
	{
		EventRoomCode.Reset();
		return;
	}

	if (const AA302PlayerState* InstigatorState = InstigatorCharacter->GetPlayerState<AA302PlayerState>())
	{
		EventRoomCode = InstigatorState->GetRoomCode();
	}
	else
	{
		EventRoomCode.Reset();
	}
}

void UBaseEvent::ExecuteEvent_Implementation(ACharacter* InstigatorCharacter)
{
	UE_LOG(LogTemp, Warning, TEXT("[Event] ExecuteEvent_Implementation entered."));

	if (!InstigatorCharacter)
	{
		return;
	}

	InitializeRuntimeContext(InstigatorCharacter);

	if (AMyPlayerController* ClientEventBridge = Cast<AMyPlayerController>(InstigatorCharacter->GetController()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Event] Client bridge found. Trying fallback event UI."));
		if (UBasePersonalEvent* PersonalEvent = Cast<UBasePersonalEvent>(this))
		{
			ClientEventBridge->SetActivePersonalEvent(PersonalEvent);
		}

		// Fallback only: concrete events should provide their own choice array.
		TArray<FText> Choices;
		ClientEventBridge->ShowPersonalEvent(
			EventID,
			EventTitle,
			EventDescription,
			Choices
		);
	}
}

void UBaseEvent::OnEventResolved_Implementation(ACharacter* TargetCharacter, bool bIsConfirmed)
{
	UE_LOG(LogTemp, Warning, TEXT("[Event] %s resolved. confirmed=%d"), *EventID.ToString(), bIsConfirmed ? 1 : 0);
}

UWorld* UBaseEvent::GetWorld() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}

	if (GetOuter())
	{
		return GetOuter()->GetWorld();
	}

	return nullptr;
}
