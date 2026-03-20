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
    // 레벨에 배치된 ASpawnArea 중 해당 룸 공간에 있는 것을 탐색합니다.
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnArea::StaticClass(), FoundActors);

    // 룸 범위 내의 SpawnArea 중 스테이지가 맞는 것 필터링
    TArray<ASpawnArea*> ValidAreas;
    for (AActor* Actor : FoundActors)
    {
        if (ASpawnArea* SpawnArea = Cast<ASpawnArea>(Actor))
        {
            const bool bInRoom = IsSpawnAreaInRoomSpace(SpawnArea, RoomCode);
            const bool bStageMatch = (SpawnArea->TargetStage == 0 || SpawnArea->TargetStage == StageNum);
            if (bInRoom && bStageMatch)
            {
                ValidAreas.Add(SpawnArea);
            }
        }
    }

    if (ValidAreas.Num() > 0)
    {
        // 유효한 SpawnArea 중 랜덤 하나 선택 → 영역 내 랜덤 위치 반환
        const int32 RandomIndex = FMath::RandRange(0, ValidAreas.Num() - 1);
        const ASpawnArea* SelectedArea = ValidAreas[RandomIndex];
        const FVector SpawnLocation = SelectedArea->GetRandomPointInBox();

        UE_LOG(
            LogTemp,
            Log,
            TEXT("[SpawnManager] SpawnArea found. area=%s location=(%.0f, %.0f, %.0f) room=%s stage=%d"),
            *GetNameSafe(SelectedArea),
            SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z,
            *RoomCode,
            StageNum
        );

        return FTransform(FRotator::ZeroRotator, SpawnLocation);
    }

    // SpawnArea를 찾지 못한 경우 룸 오프셋 원점 위로 fallback
    const FVector RoomOffset = GetRoomOffset(RoomCode);
    const FVector FallbackLoc = RoomOffset + FVector(0.f, 0.f, 500.f);

    UE_LOG(
        LogTemp,
        Warning,
        TEXT("[SpawnManager] No SpawnArea found for room=%s stage=%d. Falling back to room origin (%.0f, %.0f, %.0f). Place BP_SpawnActor in the level."),
        *RoomCode,
        StageNum,
        FallbackLoc.X, FallbackLoc.Y, FallbackLoc.Z
    );

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

