#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IntroSplashComponent.generated.h"

class UIntroSplashWidget;
class USoundBase;

/**
 * 패키지 EXE 실행 시 인트로 스플래시 화면을 관리하는 컴포넌트.
 *
 * - 로컬 클라이언트에서만 동작 (IsLocalController() 검증)
 * - 서버 실행 없음 / 리플리케이션 없음
 * - AMyPlayerController 를 수정하지 않고, BP 서브클래스에 추가하는 방식으로 부착
 *
 * 실행 순서:
 *   BeginPlay → InitDelaySeconds 대기 → 위젯 생성 & AddToViewport →
 *   FadeIn 재생 & 사운드 재생 → (FadeIn + Hold) 후 FadeOut 재생 →
 *   FadeOut 완료 후 위젯 제거
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class A302CLIENT_API UIntroSplashComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIntroSplashComponent();

	/** 인트로를 즉시 건너뜁니다. BlueprintCallable 이므로 BP 입력 이벤트에서 호출 가능 */
	UFUNCTION(BlueprintCallable, Category = "Intro")
	void SkipIntro();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ──────────────────────────────────────────────
	// 설정값 (블루프린트 서브클래스에서 에디터로 지정)
	// ──────────────────────────────────────────────

	/** WBP_IntroSplash 의 클래스 레퍼런스 (UIntroSplashWidget 을 부모로 하는 BP) */
	UPROPERTY(EditDefaultsOnly, Category = "Intro|Widget")
	TSubclassOf<UIntroSplashWidget> IntroWidgetClass;

	/** 재생할 인트로 사운드 (SoundCue 또는 SoundWave) */
	UPROPERTY(EditDefaultsOnly, Category = "Intro|Sound")
	TObjectPtr<USoundBase> IntroSound;

	/** HUD 초기화 안전 대기 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "Intro|Timing", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float InitDelaySeconds = 0.3f;

	/** FadeIn 애니메이션 재생 시간 (초) — WBP 애니메이션 길이와 일치시킬 것 */
	UPROPERTY(EditDefaultsOnly, Category = "Intro|Timing", meta = (ClampMin = "0.1"))
	float FadeInDurationSeconds = 1.5f;

	/** FadeIn 완료 후 유지 시간 (초) */
	UPROPERTY(EditDefaultsOnly, Category = "Intro|Timing", meta = (ClampMin = "0.0"))
	float HoldDurationSeconds = 2.5f;

	/** FadeOut 애니메이션 재생 시간 (초) — WBP 애니메이션 길이와 일치시킬 것 */
	UPROPERTY(EditDefaultsOnly, Category = "Intro|Timing", meta = (ClampMin = "0.1"))
	float FadeOutDurationSeconds = 1.5f;

private:
	APlayerController* GetOwnerController() const;

	void ShowIntroWidget();
	void BeginFadeOut();
	void FinishIntro();
	void ClearAllTimers();

	UPROPERTY(Transient)
	TObjectPtr<UIntroSplashWidget> IntroWidgetInstance;

	FTimerHandle InitDelayHandle;
	FTimerHandle FadeOutTriggerHandle;
	FTimerHandle FinishHandle;

	bool bIntroActive = false;
};
