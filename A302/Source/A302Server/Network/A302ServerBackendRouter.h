#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "A302ServerBackendRouter.generated.h"

class AA302GameMode;
class FJsonObject;

/**
 * 전용 서버의 백엔드 패킷 수신 및 도메인별 분발 처리를 담당하는 브로커 객체입니다.
 * GameMode의 비대화를 방지하고 패킷 처리 로직을 격리합니다.
 */
UCLASS()
class A302SERVER_API UA302ServerBackendRouter : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(AA302GameMode* InGameMode);

	/** 백엔드로부터 수신된 원시 패킷을 처리합니다. */
	void HandleMessage(const FString& Message);

	/** 게임 종료를 백엔드에 알립니다. */
	void NotifyGameFinished(const FString& RoomCode);

	/** 플레이어 로그아웃을 백엔드에 알립니다. (닉네임 점유 해제용) */
	void NotifyPlayerLogout(const FString& PlayerName, const FString& RoomCode);

private:
	/** prepare_game 패킷 처리 */
	void HandlePrepareGame(const TSharedPtr<FJsonObject>& Data);

	/** 채팅 메시지 패킷 처리 */
	void HandleChatMessage(const TSharedPtr<FJsonObject>& Data, const FString& RawMessage);

	UPROPERTY()
	TWeakObjectPtr<AA302GameMode> OwningGameMode;
};
