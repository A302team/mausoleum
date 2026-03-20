#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "A302ServerPlayerSubsystem.generated.h"

class AA302GameMode;
class APlayerController;
class AController;
class AMyCharacter;

/**
 * 전용 서버의 플레이어 상태 관리 및 스폰 조율을 전담하는 서브시스템입니다.
 * GameMode의 플레이어 관련 로직을 분산하여 유지보수성을 높입니다.
 */
UCLASS()
class A302SERVER_API UA302ServerPlayerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 플레이어 접속 시 초기화 및 방 할당 조율 */
	void HandlePlayerLogin(APlayerController* NewPlayer);

	/** 플레이어 퇴장 시 정리 */
	void HandlePlayerLogout(AController* Exiting);

	/** 플레이어의 게임플레이 활성 상태를 확인 */
	bool IsPlayerGameplayEnabled(const APlayerController* PlayerController) const;

	/** 플레이어의 게임플레이 플래그 갱신 (PlayerState 동기화) */
	void UpdatePlayerGameplayFlag(APlayerController* PlayerController, bool bEnabled);

	/** 플레이어를 스폰 큐에 등록 */
	void QueueSpawnPlayer(APlayerController* PlayerController, bool bForceSpawn = false);

private:
    UFUNCTION()
    void OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

    void OnRequestMaliceAnnouncement(const FString& PlayerName, int32 MaliceCount, AMyCharacter* SourceCharacter);

	/** 내부 조율을 위한 GameMode 획득 */
	AA302GameMode* GetA302GameMode() const;
};
