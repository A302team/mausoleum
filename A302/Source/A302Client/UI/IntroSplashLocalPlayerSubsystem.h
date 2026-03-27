#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UObject/SoftObjectPtr.h"
#include "IntroSplashLocalPlayerSubsystem.generated.h"

class APlayerController;
class UIntroSplashWidget;
class USoundBase;

UCLASS()
class A302CLIENT_API UIntroSplashLocalPlayerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void PlayerControllerChanged(APlayerController* NewPlayerController) override;

private:
	void TryScheduleIntro(APlayerController* PlayerController);
	void ShowIntroWidget();
	void BeginFadeOut();
	void FinishIntro();
	void ClearTimers();

	UPROPERTY(EditDefaultsOnly, Category = "Intro")
	TSoftClassPtr<UIntroSplashWidget> IntroWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Intro")
	TSoftObjectPtr<USoundBase> IntroSound;

	UPROPERTY(EditDefaultsOnly, Category = "Intro", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float InitDelaySeconds = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Intro", meta = (ClampMin = "0.1"))
	float FadeInDurationSeconds = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Intro", meta = (ClampMin = "0.0"))
	float HoldDurationSeconds = 2.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Intro", meta = (ClampMin = "0.1"))
	float FadeOutDurationSeconds = 1.5f;

	UPROPERTY(Transient)
	TObjectPtr<UIntroSplashWidget> IntroWidgetInstance = nullptr;

	TWeakObjectPtr<UWorld> ActiveWorld;
	FTimerHandle InitDelayHandle;
	FTimerHandle FadeOutTriggerHandle;
	FTimerHandle FinishHandle;

	bool bIntroQueued = false;
	bool bIntroShown = false;
};
