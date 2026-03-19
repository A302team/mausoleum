// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/SpawnManager.h"
#include "GameMode/A302PlayerState.h"
#include "Object/SpawnArea.h"
#include "Room/RoomWorldOffset.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

ASpawnManager::ASpawnManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

ASpawnManager* ASpawnManager::FindOrSpawn(UWorld* World)
{
    if (!World)
    {
        return nullptr;
    }

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(World, ASpawnManager::StaticClass(), FoundActors);
    for (AActor* FoundActor : FoundActors)
    {
        if (ASpawnManager* FoundManager = Cast<ASpawnManager>(FoundActor))
        {
            return FoundManager;
        }
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    return World->SpawnActor<ASpawnManager>(ASpawnManager::StaticClass(), FTransform::Identity, SpawnParams);
}

void ASpawnManager::QueueSpawnAndPossessPlayer(APlayerController* PlayerController, TSubclassOf<ACharacter> CharacterClass, int32 StageNum, const FString& RoomCode, float WarmupSeconds)
{
    if (!PlayerController)
    {
        return;
    }

    const TSubclassOf<ACharacter> CapturedCharacterClass = CharacterClass;
    const FString CapturedRoomCode = RoomCode;
    const float SafeWarmupSeconds = FMath::Max(0.0f, WarmupSeconds);
    if (SafeWarmupSeconds <= KINDA_SMALL_NUMBER)
    {
        SpawnAndPossessPlayer(PlayerController, CapturedCharacterClass, StageNum, CapturedRoomCode);
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        SpawnAndPossessPlayer(PlayerController, CapturedCharacterClass, StageNum, CapturedRoomCode);
        return;
    }

    TWeakObjectPtr<ASpawnManager> WeakThis(this);
    TWeakObjectPtr<APlayerController> WeakPlayerController(PlayerController);

    FTimerHandle DeferredSpawnHandle;
    World->GetTimerManager().SetTimer(
        DeferredSpawnHandle,
        FTimerDelegate::CreateWeakLambda(this, [WeakThis, WeakPlayerController, CapturedCharacterClass, StageNum, CapturedRoomCode]()
        {
            if (!WeakThis.IsValid() || !WeakPlayerController.IsValid())
            {
                return;
            }

            APlayerController* DeferredPlayer = WeakPlayerController.Get();
            if (!DeferredPlayer)
            {
                return;
            }

            if (const AA302PlayerState* A302PlayerState = DeferredPlayer->GetPlayerState<AA302PlayerState>())
            {
                if (!A302PlayerState->bGameplayEnabled)
                {
                    return;
                }
            }

            WeakThis->SpawnAndPossessPlayer(DeferredPlayer, CapturedCharacterClass, StageNum, CapturedRoomCode);
        }),
        SafeWarmupSeconds,
        false
    );
}

bool ASpawnManager::SpawnAndPossessPlayer(APlayerController* PlayerController, TSubclassOf<ACharacter> CharacterClass, int32 StageNum, const FString& RoomCode)
{
    if (!PlayerController)
    {
        return false;
    }

    UClass* SpawnClass = CharacterClass.Get();
    if (!SpawnClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SpawnManager] SpawnPlayer failed: CharacterClass is null. room=%s"), *RoomCode);
        return false;
    }

    APawn* ExistingPawn = PlayerController->GetPawn();
    if (ExistingPawn)
    {
        if (ExistingPawn->IsA(SpawnClass))
        {
            return true;
        }

        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[SpawnManager] Replacing existing pawn before spawn. controller=%s existing=%s targetClass=%s room=%s"),
            *GetNameSafe(PlayerController),
            *GetNameSafe(ExistingPawn),
            *GetNameSafe(SpawnClass),
            *RoomCode
        );

        PlayerController->UnPossess();
        ExistingPawn->Destroy();
    }

    const FTransform SpawnTransform = GetRandomPlayerSpawnTransform(StageNum, RoomCode);

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    ACharacter* Character = GetWorld()->SpawnActor<ACharacter>(SpawnClass, SpawnTransform, SpawnParams);
    if (!Character)
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[SpawnManager] SpawnActor failed. room=%s class=%s location=(%.0f, %.0f, %.0f)"),
            *RoomCode,
            *GetNameSafe(SpawnClass),
            SpawnTransform.GetLocation().X,
            SpawnTransform.GetLocation().Y,
            SpawnTransform.GetLocation().Z
        );
        return false;
    }

    Character->SetReplicates(true);
    Character->SetReplicateMovement(true);
    PlayerController->Possess(Character);

    if (PlayerController->GetPawn() != Character)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[SpawnManager] Possess failed. controller=%s pawn=%s expected=%s room=%s"),
            *GetNameSafe(PlayerController),
            *GetNameSafe(PlayerController->GetPawn()),
            *GetNameSafe(Character),
            *RoomCode
        );
        return false;
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[SpawnManager] Spawn+Possess success. controller=%s pawn=%s room=%s stage=%d"),
        *GetNameSafe(PlayerController),
        *GetNameSafe(Character),
        *RoomCode,
        StageNum
    );

    return true;
}

FTransform ASpawnManager::GetRandomPlayerSpawnTransform(int32 StageNum, const FString& RoomCode) const
{
    // 1. 월드에 배치된 모든 SpawnArea 찾기
    TArray<AActor*> AllAreas;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnArea::StaticClass(), AllAreas);

    // 2. Stage 우선순위: exact(StageNum) -> fallback(Stage 0) -> any stage
    TArray<ASpawnArea*> AllSpawnAreas;
    TArray<ASpawnArea*> ExactStageAreas;
    TArray<ASpawnArea*> FallbackStageAreas;
    for (AActor* Actor : AllAreas)
    {
        if (ASpawnArea* Area = Cast<ASpawnArea>(Actor))
        {
            AllSpawnAreas.Add(Area);
            if (Area->TargetStage == StageNum)
            {
                ExactStageAreas.Add(Area);
            }
            else if (Area->TargetStage == 0)
            {
                FallbackStageAreas.Add(Area);
            }
        }
    }

    const TArray<ASpawnArea*>* SelectedStageAreas = nullptr;
    const TCHAR* StageSelectMode = TEXT("exact");
    if (ExactStageAreas.Num() > 0)
    {
        SelectedStageAreas = &ExactStageAreas;
    }
    else if (FallbackStageAreas.Num() > 0)
    {
        SelectedStageAreas = &FallbackStageAreas;
        StageSelectMode = TEXT("stage0_fallback");
    }
    else if (AllSpawnAreas.Num() > 0)
    {
        SelectedStageAreas = &AllSpawnAreas;
        StageSelectMode = TEXT("any_stage_fallback");
    }

    // 영역이 하나도 배치되지 않았다면 기본값 반환 (방어적 코드)
    if (!SelectedStageAreas)
    {
        const FVector RoomOffset = GetRoomOffset(RoomCode);
        UE_LOG(
            LogTemp,
            Error,
            TEXT("[SpawnManager] SpawnArea missing entirely. stage=%d room=%s total=%d. use safe fallback above room."),
            StageNum,
            RoomCode.IsEmpty() ? TEXT("none") : *RoomCode,
            AllAreas.Num()
        );
        return FTransform(FRotator::ZeroRotator, RoomOffset + FVector(0.0, 0.0, 3000.0));
    }

    if (!FCString::Strcmp(StageSelectMode, TEXT("exact")))
    {
        UE_LOG(LogTemp, Verbose, TEXT("[SpawnManager] stage select exact. stage=%d room=%s count=%d"), StageNum, *RoomCode, SelectedStageAreas->Num());
    }
    else
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("[SpawnManager] stage select fallback mode=%s requestedStage=%d room=%s count=%d"),
            StageSelectMode,
            StageNum,
            *RoomCode,
            SelectedStageAreas->Num()
        );
    }

    TArray<ASpawnArea*> RoomScopedAreas;
    for (ASpawnArea* Area : *SelectedStageAreas)
    {
        if (IsSpawnAreaInRoomSpace(Area, RoomCode))
        {
            RoomScopedAreas.Add(Area);
        }
    }

    const bool bUseLegacyOffset = (RoomScopedAreas.Num() == 0);
    const TArray<ASpawnArea*>& CandidateAreas = bUseLegacyOffset ? *SelectedStageAreas : RoomScopedAreas;
    const FVector RoomOffset = GetRoomOffset(RoomCode);
    UE_LOG(
        LogTemp,
        Verbose,
        TEXT("[SpawnManager] room=%s stage=%d mode=%s offset=(%.0f, %.0f, %.0f)"),
        RoomCode.IsEmpty() ? TEXT("none") : *RoomCode,
        StageNum,
        bUseLegacyOffset ? TEXT("legacy_offset") : TEXT("instanced_level"),
        RoomOffset.X,
        RoomOffset.Y,
        RoomOffset.Z
    );

    // 3. 무작위 영역에서 충돌 없는(안전한) 좌표 찾기 시도 (최대 20번)
    int32 MaxAttempts = 20;
    for (int32 i = 0; i < MaxAttempts; ++i)
    {
        // 유효한 영역 중 하나를 무작위로 선택
        ASpawnArea* SelectedArea = CandidateAreas[FMath::RandRange(0, CandidateAreas.Num() - 1)];
        FVector CandidateLoc = SelectedArea->GetRandomPointInBox();
        if (bUseLegacyOffset)
        {
            CandidateLoc += RoomOffset;
        }

        // 다른 플레이어와 겹치지 않는다면 최종 확정
        if (IsLocationSafe(CandidateLoc))
        {
            FRotator RandomRot(0.f, FMath::RandRange(0.f, 360.f), 0.f); // 시선 방향은 무작위
            return FTransform(RandomRot, CandidateLoc);
        }
    }

    // 20번 시도에도 꽉 차서 실패했다면, 그냥 첫 번째 영역의 무작위 위치로 강제 스폰
    UE_LOG(LogTemp, Warning, TEXT("[SpawnManager] 안전한 빈 공간을 찾지 못했습니다. 강제 스폰을 진행합니다."));
    FVector FallbackLoc = CandidateAreas[0]->GetRandomPointInBox();
    if (bUseLegacyOffset)
    {
        FallbackLoc += RoomOffset;
    }
    return FTransform(FRotator::ZeroRotator, FallbackLoc);
}

bool ASpawnManager::IsLocationSafe(const FVector& Location) const
{
    // 플레이어 캡슐 크기에 맞춘 검사 영역
    FCollisionShape Capsule = FCollisionShape::MakeCapsule(50.f, 90.f);
    
    // 이 위치에 Pawn(플레이어 캐릭터)이 하나라도 겹쳐있는지 확인
    bool bHasOverlap = GetWorld()->OverlapAnyTestByObjectType(
        Location, 
        FQuat::Identity, 
        FCollisionObjectQueryParams(ECollisionChannel::ECC_Pawn), 
        Capsule
    );

    return !bHasOverlap; // 겹치는 게 없어야(false) 안전함(true) 반환
}

FVector ASpawnManager::GetRoomOffset(const FString& RoomCode) const
{
    return A302RoomWorldOffset::ResolveRoomOffset(RoomCode);
}

bool ASpawnManager::IsSpawnAreaInRoomSpace(const ASpawnArea* SpawnArea, const FString& RoomCode) const
{
    if (!SpawnArea)
    {
        return false;
    }

    const FVector RoomOffset = GetRoomOffset(RoomCode);
    const double DistanceX = FMath::Abs(SpawnArea->GetActorLocation().X - RoomOffset.X);
    const double AcceptRangeX = A302RoomWorldOffset::DefaultOffsetStepX * 0.45;
    return DistanceX <= AcceptRangeX;
}

