#include "Subsystem/A302ServerPlayerSubsystem.h"
#include "GameMode/A302GameMode.h"
#include "GameMode/A302PlayerState.h"
#include "Room/RoomMembershipRegistry.h"
#include "Room/RoomScopeRules.h"
#include "Room/A302RoomRuntimeSubsystem.h"
#include "Phase/A302ServerPhaseSubsystem.h"
#include "Subsystem/A302RoomEventSubsystem.h"
#include "Manager/SpawnManager.h"
#include "GameFramework/PlayerController.h"
#include "Character/MyCharacter.h"
#include "Engine/World.h"

namespace
{
    bool IsLocalPieSession(const UWorld* World)
    {
#if WITH_EDITOR
        return World && World->WorldType == EWorldType::PIE && World->GetNetMode() != NM_DedicatedServer;
#else
        return false;
#endif
    }
}

void UA302ServerPlayerSubsystem::HandlePlayerLogin(APlayerController* NewPlayer)
{
    AA302GameMode* GM = GetA302GameMode();
    if (!GM || !NewPlayer) return;

    URoomMembershipRegistry* Registry = GM->GetRoomMembershipRegistry();
    if (Registry)
    {
        Registry->AssignPlayerToRoom(NewPlayer);
    }

    const bool bEnabled = IsPlayerGameplayEnabled(NewPlayer);
    UpdatePlayerGameplayFlag(NewPlayer, bEnabled);

    if (bEnabled)
    {
        QueueSpawnPlayer(NewPlayer);
    }

    // Pawn possession 감시
    NewPlayer->OnPossessedPawnChanged.RemoveDynamic(this, &UA302ServerPlayerSubsystem::OnPossessedPawnChanged);
    NewPlayer->OnPossessedPawnChanged.AddDynamic(this, &UA302ServerPlayerSubsystem::OnPossessedPawnChanged);
    
    // 현재 Pawn이 이미 있으면 즉시 바인딩 시도
    if (NewPlayer->GetPawn())
    {
        OnPossessedPawnChanged(nullptr, NewPlayer->GetPawn());
    }
}

void UA302ServerPlayerSubsystem::HandlePlayerLogout(AController* Exiting)
{
    AA302GameMode* GM = GetA302GameMode();
    if (!GM) return;

    APlayerController* ExitingPC = Cast<APlayerController>(Exiting);
    if (ExitingPC)
    {
        FString LeavingRoomCode;
        URoomMembershipRegistry* Registry = GM->GetRoomMembershipRegistry();
        if (Registry)
        {
            LeavingRoomCode = Registry->GetPlayerRoomCode(ExitingPC);
            Registry->ClearPendingRoomCode(ExitingPC);
        }

        LeavingRoomCode = A302RoomScope::NormalizeRoomCode(LeavingRoomCode);
        if (!LeavingRoomCode.IsEmpty() && Registry && GetWorld())
        {
            // 방에 남은 플레이어가 없으면 런타임 중지
            if (Registry->CountPlayersInRoom(GetWorld(), LeavingRoomCode) <= 0)
            {
            if (UA302RoomRuntimeSubsystem* RoomRuntime = GetWorld()->GetGameInstance()->GetSubsystem<UA302RoomRuntimeSubsystem>())
                {
                    RoomRuntime->StopRoomRuntime(LeavingRoomCode);
                }
            }
        }
    }
}

bool UA302ServerPlayerSubsystem::IsPlayerGameplayEnabled(const APlayerController* PlayerController) const
{
    AA302GameMode* GM = GetA302GameMode();
    if (!GM || !PlayerController) return false;

    URoomMembershipRegistry* Registry = GM->GetRoomMembershipRegistry();
    if (!Registry) return false;

    const FString RoomCode = A302RoomScope::NormalizeRoomCode(Registry->GetPlayerRoomCode(PlayerController));
    if (RoomCode.IsEmpty()) return false;

    if (IsLocalPieSession(GetWorld()) && RoomCode.StartsWith(TEXT("PIE_LOCAL_")))
    {
        return true;
    }

    // GameMode의 공개 메서드를 통해 룸 상태 확인
    return GM->IsRoomGameplayActive(RoomCode) && 
           GetWorld()->GetGameInstance()->GetSubsystem<UA302RoomRuntimeSubsystem>() && 
           GetWorld()->GetGameInstance()->GetSubsystem<UA302RoomRuntimeSubsystem>()->IsRoomLevelReady(RoomCode);
}

void UA302ServerPlayerSubsystem::UpdatePlayerGameplayFlag(APlayerController* PlayerController, bool bEnabled)
{
    if (!PlayerController) return;

    if (AA302PlayerState* PS = PlayerController->GetPlayerState<AA302PlayerState>())
    {
        PS->SetGameplayEnabled(bEnabled);
    }
}

void UA302ServerPlayerSubsystem::QueueSpawnPlayer(APlayerController* PlayerController, bool bForceSpawn)
{
    AA302GameMode* GM = GetA302GameMode();
    if (!GM || !PlayerController) return;

    URoomMembershipRegistry* Registry = GM->GetRoomMembershipRegistry();
    FString PlayerRoomCode = Registry ? A302RoomScope::NormalizeRoomCode(Registry->GetPlayerRoomCode(PlayerController)) : FString();

    if (PlayerRoomCode.IsEmpty())
    {
        if (const AA302PlayerState* PS = PlayerController->GetPlayerState<AA302PlayerState>())
        {
            PlayerRoomCode = A302RoomScope::NormalizeRoomCode(PS->GetRoomCode());
        }
    }

    UA302RoomRuntimeSubsystem* RoomRuntime = GetWorld() ? GetWorld()->GetGameInstance()->GetSubsystem<UA302RoomRuntimeSubsystem>() : nullptr;
    const bool bPhaseActive = !PlayerRoomCode.IsEmpty() && GM->IsRoomGameplayActive(PlayerRoomCode);
    const bool bRoomReady = !PlayerRoomCode.IsEmpty() && RoomRuntime && RoomRuntime->IsRoomLevelReady(PlayerRoomCode);
    const bool bGameplayEnabled = bPhaseActive && bRoomReady;

    if (!bForceSpawn && !bGameplayEnabled)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[ServerPlayerSubsystem] QueueSpawnPlayer blocked. player=%s room=%s phaseActive=%s roomReady=%s"),
            *GetNameSafe(PlayerController),
            *PlayerRoomCode,
            bPhaseActive ? TEXT("true") : TEXT("false"),
            bRoomReady ? TEXT("true") : TEXT("false")
        );
        return;
    }

    if (bForceSpawn && !bGameplayEnabled)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[ServerPlayerSubsystem] QueueSpawnPlayer forced despite inactive state. player=%s room=%s phaseActive=%s roomReady=%s"),
            *GetNameSafe(PlayerController),
            *PlayerRoomCode,
            bPhaseActive ? TEXT("true") : TEXT("false"),
            bRoomReady ? TEXT("true") : TEXT("false")
        );
    }

    ASpawnManager* SpawnManager = ASpawnManager::FindOrSpawn(GetWorld());
    if (SpawnManager)
    {
        SpawnManager->QueueSpawnAndPossessPlayer(
            PlayerController,
            GM->CharacterClass,
            1, // Default Stage
            PlayerRoomCode,
            3.0f // 레벨 스트리밍 후 물리 콜리전 초기화 대기 (1.0f → 3.0f)
        );
        UE_LOG(
            LogTemp,
            Log,
            TEXT("[ServerPlayerSubsystem] QueueSpawnPlayer queued. player=%s room=%s force=%s"),
            *GetNameSafe(PlayerController),
            *PlayerRoomCode,
            bForceSpawn ? TEXT("true") : TEXT("false")
        );
    }
    else
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[ServerPlayerSubsystem] QueueSpawnPlayer failed: SpawnManager missing. player=%s room=%s"),
            *GetNameSafe(PlayerController),
            *PlayerRoomCode
        );
    }
}

void UA302ServerPlayerSubsystem::OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
    if (AMyCharacter* OldCharacter = Cast<AMyCharacter>(OldPawn))
    {
        OldCharacter->OnRequestPublicMaliceAnnouncement.RemoveAll(this);
    }

    if (AMyCharacter* NewCharacter = Cast<AMyCharacter>(NewPawn))
    {
        NewCharacter->OnRequestPublicMaliceAnnouncement.RemoveAll(this);
        NewCharacter->OnRequestPublicMaliceAnnouncement.AddUObject(this, &UA302ServerPlayerSubsystem::OnRequestMaliceAnnouncement, NewCharacter);
    }
}

void UA302ServerPlayerSubsystem::OnRequestMaliceAnnouncement(const FString& PlayerName, int32 MaliceCount, AMyCharacter* SourceCharacter)
{
    if (!SourceCharacter) return;

    if (AA302PlayerState* PS = SourceCharacter->GetPlayerState<AA302PlayerState>())
    {
        if (UWorld* World = GetWorld())
        {
            if (UA302RoomEventSubsystem* EventSubsystem = World->GetSubsystem<UA302RoomEventSubsystem>())
            {
                EventSubsystem->BroadcastMaliceAnnouncementToRoom(PS->GetRoomCode(), PlayerName, MaliceCount);
            }
        }
    }
}

AA302GameMode* UA302ServerPlayerSubsystem::GetA302GameMode() const
{
    return GetWorld() ? Cast<AA302GameMode>(GetWorld()->GetAuthGameMode()) : nullptr;
}
