#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Animation/WidgetAnimation.h"
#include "IntroSplashWidget.generated.h"

/**
 * WBP_IntroSplash 의 C++ 베이스 클래스.
 * 블루프린트에서 반드시 FadeInAnim / FadeOutAnim 애니메이션을 동일한 이름으로 생성해야 합니다.
 * UIntroSplashComponent 가 이 클래스를 통해 페이드 시퀀스를 제어합니다.
 */
UCLASS(Abstract)
class A302CLIENT_API UIntroSplashWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** FadeIn 애니메이션 재생 (opacity 0 → 1) */
	void PlayFadeIn();

	/** FadeOut 애니메이션 재생 (opacity 1 → 0) */
	void PlayFadeOut();

protected:
	/** 블루프린트에서 "FadeInAnim" 으로 생성된 애니메이션과 자동 바인딩 */
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> FadeInAnim;

	/** 블루프린트에서 "FadeOutAnim" 으로 생성된 애니메이션과 자동 바인딩 */
	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> FadeOutAnim;
};
