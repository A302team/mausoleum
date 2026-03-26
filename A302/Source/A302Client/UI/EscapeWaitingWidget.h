#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EscapeWaitingWidget.generated.h"

class UTextBlock;
class UWidget;

/**
 * 탈출 성공 후 다른 플레이어를 기다리는 동안 표시되는 UI 위젯.
 *
 * Blueprint(WBP_EscapeWaiting)에서 이 클래스를 부모로 설정한 뒤,
 * 아래 BindWidget 변수 이름과 정확히 일치하는 위젯을 배치하면 됩니다.
 * 모든 로직은 C++에서 처리하므로 Blueprint Event Graph는 비워도 됩니다.
 */
UCLASS()
class A302CLIENT_API UEscapeWaitingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ──────────────────────────────────────────────────────────────────────
	// BindWidget: Blueprint에서 반드시 아래 이름과 동일한 위젯을 배치하세요.
	// ──────────────────────────────────────────────────────────────────────

	/** "탈출 성공!" 메인 타이틀 텍스트 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_EscapeSuccess;

	/** "다른 플레이어를 기다리는 중..." 안내 텍스트 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_WaitingMessage;

	/** 현재 관전 중인 플레이어 이름 표시 텍스트 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_SpectatorTargetName;

	/**
	 * 관전 안내 영역 전체 패널.
	 * 관전할 대상이 없으면 Collapsed, 있으면 Visible.
	 * (SizeBox, Overlay, VerticalBox 등 아무 패널이나 가능)
	 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWidget> Panel_SpectatorInfo;

	// ──────────────────────────────────────────────────────────────────────
	// C++ 로직 함수 (HUD에서 호출)
	// ──────────────────────────────────────────────────────────────────────

	/** 관전 중인 플레이어 이름 갱신. 빈 문자열이면 Panel_SpectatorInfo를 Collapsed */
	void UpdateSpectatorTargetName(const FString& TargetName);
};
