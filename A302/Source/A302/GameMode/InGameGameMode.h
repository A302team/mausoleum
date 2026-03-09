#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "InGameGameMode.generated.h"

/**
 * @brief 게임 플레이 중에 적용되는 게임 모드 클래스. AGameModeBase를 상속받아 플레이어의 스폰 위치를 커스텀하게 결정하는 기능을 제공합니다.
 * @note 이 클래스는 플레이어가 게임에 참여할 때마다 SpawnManager를 통해 안전한 스폰 위치를 반환받아 스폰하도록 구현되어 있습니다.
 * @author 홍윤표
 */
UCLASS()
class A302_API AInGameGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AInGameGameMode();

    // 엔진의 기본 스폰 프로세스를 가로채어 커스텀 로직 적용
    virtual void RestartPlayer(AController* NewPlayer) override;

private:
    // 월드에 배치된 SpawnManager를 가져오거나 생성하는 헬퍼 함수
    class ASpawnManager* GetSpawnManager();

    // 캐싱용 매니저 포인터
    UPROPERTY()
    class ASpawnManager* CachedSpawnManager;
};
