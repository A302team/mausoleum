#include "GameMode/InGameGameMode.h"
#include "Manager/SpawnManager.h"
#include "Kismet/GameplayStatics.h"

AInGameGameMode::AInGameGameMode()
{
    // DefaultPawnClass 등은 BP_InGameGameMode 에서 지정
}

void AInGameGameMode::RestartPlayer(AController* NewPlayer)
{
    if (!NewPlayer || NewPlayer->IsPendingKillPending()) return;

    if (ASpawnManager* SM = GetSpawnManager())
    {
        // 0단계(초기 시작)용 스폰 좌표를 계산하라고 매니저에게 요청
        FTransform SpawnTransform = SM->GetRandomPlayerSpawnTransform(0);
        
        // 해당 트랜스폼에 캐릭터 생성 및 빙의(Possess)
        RestartPlayerAtTransform(NewPlayer, SpawnTransform);
    }
    else
    {
        // 만약 맵에 매니저가 없다면 엔진 기본 방식 따름
        Super::RestartPlayer(NewPlayer);
    }
}

ASpawnManager* AInGameGameMode::GetSpawnManager()
{
    if (CachedSpawnManager) return CachedSpawnManager;

    // 월드에서 매니저를 찾아서 캐싱
    CachedSpawnManager = Cast<ASpawnManager>(
        UGameplayStatics::GetActorOfClass(GetWorld(), ASpawnManager::StaticClass())
    );

    // 에디터에서 깜빡하고 안 배치했을 경우를 대비해 런타임에 자동 생성 (옵션)
    if (!CachedSpawnManager)
    {
        CachedSpawnManager = GetWorld()->SpawnActor<ASpawnManager>(ASpawnManager::StaticClass());
    }

    return CachedSpawnManager;
}
