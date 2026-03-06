// Fill out your copyright notice in the Description page of Project Settings.


#include "Manager/SpawnManager.h"
#include "Object/SpawnArea.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

ASpawnManager::ASpawnManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

FTransform ASpawnManager::GetRandomPlayerSpawnTransform(int32 StageNum) const
{
    // 1. 월드에 배치된 모든 SpawnArea 찾기
    TArray<AActor*> AllAreas;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ASpawnArea::StaticClass(), AllAreas);

    // 2. 현재 요청된 스테이지(StageNum)와 일치하는 영역만 추려내기
    TArray<ASpawnArea*> ValidAreas;
    for (AActor* Actor : AllAreas)
    {
        if (ASpawnArea* Area = Cast<ASpawnArea>(Actor))
        {
            if (Area->TargetStage == StageNum)
            {
                ValidAreas.Add(Area);
            }
        }
    }

    // 영역이 하나도 배치되지 않았다면 기본값 반환 (방어적 코드)
    if (ValidAreas.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("[SpawnManager] Stage %d 에 해당하는 SpawnArea가 없습니다!"), StageNum);
        return FTransform::Identity;
    }

    // 3. 무작위 영역에서 충돌 없는(안전한) 좌표 찾기 시도 (최대 20번)
    int32 MaxAttempts = 20;
    for (int32 i = 0; i < MaxAttempts; ++i)
    {
        // 유효한 영역 중 하나를 무작위로 선택
        ASpawnArea* SelectedArea = ValidAreas[FMath::RandRange(0, ValidAreas.Num() - 1)];
        FVector CandidateLoc = SelectedArea->GetRandomPointInBox();

        // 다른 플레이어와 겹치지 않는다면 최종 확정
        if (IsLocationSafe(CandidateLoc))
        {
            FRotator RandomRot(0.f, FMath::RandRange(0.f, 360.f), 0.f); // 시선 방향은 무작위
            return FTransform(RandomRot, CandidateLoc);
        }
    }

    // 20번 시도에도 꽉 차서 실패했다면, 그냥 첫 번째 영역의 무작위 위치로 강제 스폰
    UE_LOG(LogTemp, Warning, TEXT("[SpawnManager] 안전한 빈 공간을 찾지 못했습니다. 강제 스폰을 진행합니다."));
    FVector FallbackLoc = ValidAreas[0]->GetRandomPointInBox();
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
