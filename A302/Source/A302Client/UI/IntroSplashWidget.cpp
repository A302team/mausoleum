#include "UI/IntroSplashWidget.h"

void UIntroSplashWidget::PlayFadeIn()
{
	if (FadeInAnim)
	{
		PlayAnimation(FadeInAnim);
	}
}

void UIntroSplashWidget::PlayFadeOut()
{
	if (FadeOutAnim)
	{
		PlayAnimation(FadeOutAnim);
	}
}
