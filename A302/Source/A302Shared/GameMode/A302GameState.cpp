// Fill out your copyright notice in the Description page of Project Settings.

#include "GameMode/A302GameState.h"

#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Object/StatueInteractable.h"
#include "Character/MyPlayerController.h"

AA302GameState::AA302GameState()
{
	LastNotifiedGamePhase = GamePhase;
}

void AA302GameState::SetGamePhase(EGamePhase NewGamePhase, float ChangedServerTime, const FString& RoomCode)
{
	if (GamePhase == NewGamePhase && FMath::IsNearlyEqual(PhaseChangedServerTime, ChangedServerTime))
	{
		return;
	}

    const EGamePhase PreviousPhase = LastNotifiedGamePhase;
    GamePhase = NewGamePhase;
    PhaseChangedServerTime = ChangedServerTime;
    GamePhaseRoomCode = RoomCode;
    LastNotifiedGamePhase = GamePhase;

	GamePhaseChangedDelegate.Broadcast(PreviousPhase, GamePhase, PhaseChangedServerTime);
	if (GamePhase == EGamePhase::Ended)
	{
		HandlePhaseEnded();
	}
}

void AA302GameState::SetMatchTimer(float StartServerTime, float TimeLimitSeconds)
{
	MatchStartServerTime = StartServerTime;
	MatchTimeLimitSeconds = TimeLimitSeconds;
}

void AA302GameState::OnRep_MatchStartServerTime()
{
    // 서버가 타이머를 시작한 시점(0→양수)에만 동작
    if (MatchStartServerTime <= 0.0f || MatchTimeLimitSeconds <= 0.0f)
    {
        return;
    }

    // Client_ConfigureMatchTimer RPC를 놓쳤거나 HUD 미초기화 상태에서 도착한 경우 복구.
    // 이미 RPC로 타이머가 설정되었더라도 동일한 값을 다시 적용하므로 중복 실행에 안전.
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        if (AMyPlayerController* PC = Cast<AMyPlayerController>(It->Get()))
        {
            if (PC->IsLocalController())
            {
                PC->ConfigureMatchTimer(MatchStartServerTime, MatchTimeLimitSeconds, true);
                break;
            }
        }
    }
}

void AA302GameState::OnRep_GamePhase()
{
    const EGamePhase PreviousPhase = LastNotifiedGamePhase;
    LastNotifiedGamePhase = GamePhase;
    // 이 OnRep에서 처리한 RoomCode를 기록 → OnRep_GamePhaseRoomCode 중복 발화 방지
    LastNotifiedRoomCode = GamePhaseRoomCode;

	if (PreviousPhase != GamePhase)
	{
		GamePhaseChangedDelegate.Broadcast(PreviousPhase, GamePhase, PhaseChangedServerTime);
		if (GamePhase == EGamePhase::Ended)
		{
			HandlePhaseEnded();
		}
	}
}

void AA302GameState::OnRep_GamePhaseRoomCode()
{
    // OnRep_GamePhase가 이미 처리한 경우(GamePhase 값도 같이 바뀐 경우) 중복 발화 방지.
    // GetLifetimeReplicatedProps 에서 GamePhase가 GamePhaseRoomCode보다 먼저 선언되어
    // OnRep_GamePhase가 항상 먼저 호출되고 LastNotifiedRoomCode를 업데이트하므로 안전함.
    if (LastNotifiedRoomCode == GamePhaseRoomCode)
    {
        return;
    }

    // 여기까지 도달 = GamePhase 값은 변하지 않았지만 RoomCode가 바뀐 경우
    // (다른 방이 동일한 페이즈 번호로 전환) → 해당 방 플레이어에게 UI 표시를 위해 Broadcast.
    LastNotifiedRoomCode = GamePhaseRoomCode;
    LastNotifiedGamePhase = GamePhase;

    GamePhaseChangedDelegate.Broadcast(GamePhase, GamePhase, PhaseChangedServerTime);
}

void AA302GameState::OnRep_GamePhaseRoomCode()
{
    // 모든 미완료 석상들을 찾아서 100%로 강제 완료 처리 (서버 전용)
    if (HasAuthority())
    {
        TArray<AActor*> FoundStatues;
        UGameplayStatics::GetAllActorsOfClass(this, AStatueInteractable::StaticClass(), FoundStatues);
        for (AActor* Actor : FoundStatues)
        {
            if (AStatueInteractable* Statue = Cast<AStatueInteractable>(Actor))
            {
                Statue->ForceComplete();
            }
        }
    }

    // Stage3ClearOff 태그를 가진 액터들을 찾아서 탈출구를 개방
    TArray<AActor*> TargetActors;
    UGameplayStatics::GetAllActorsWithTag(this, TEXT("Stage3ClearOff"), TargetActors);

    // 물리적 충돌은 더 이상 막지 않도록 서버에서 끕니다
    if (HasAuthority())
    {
        for (AActor* Actor : TargetActors)
        {
            if (Actor)
            {
                Actor->SetActorEnableCollision(false);
            }
        }
    }

    // 클라이언트와 서버 모두 BP 이벤트 호출 (BP에서 Local Fog 볼륨 등의 시각적 페이드아웃 처리)
    Bp_OnEscapeUnlocked(TargetActors);
}

void AA302GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AA302GameState, GamePhase);
    DOREPLIFETIME(AA302GameState, GamePhaseRoomCode);
    DOREPLIFETIME(AA302GameState, AlivePlayerCount);
    DOREPLIFETIME(AA302GameState, PhaseChangedServerTime);
    DOREPLIFETIME(AA302GameState, MatchStartServerTime);
    DOREPLIFETIME(AA302GameState, MatchTimeLimitSeconds);
}
