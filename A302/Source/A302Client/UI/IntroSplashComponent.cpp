#include "UI/IntroSplashComponent.h"
#include "UI/IntroSplashWidget.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

UIntroSplashComponent::UIntroSplashComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 리플리케이션 비활성화 — 클라이언트 전용 컴포넌트
	SetIsReplicatedByDefault(false);
}

// ─────────────────────────────────────────────────────────────────────────────
// 생명주기
// ─────────────────────────────────────────────────────────────────────────────

void UIntroSplashComponent::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetOwnerController();
	if (!PC)
	{
		return;
	}

	// 로컬 클라이언트에서만 실행 — 서버 및 원격 클라이언트는 여기서 종료
	if (!PC->IsLocalController())
	{
		return;
	}

	if (!IntroWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("UIntroSplashComponent: IntroWidgetClass 가 설정되지 않았습니다."));
		return;
	}

	// HUD 초기화 완료 후 위젯을 생성하도록 짧게 지연
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			InitDelayHandle,
			this,
			&UIntroSplashComponent::ShowIntroWidget,
			InitDelaySeconds,
			false
		);
	}
}

void UIntroSplashComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearAllTimers();

	if (IntroWidgetInstance)
	{
		IntroWidgetInstance->RemoveFromParent();
		IntroWidgetInstance = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

// ─────────────────────────────────────────────────────────────────────────────
// 내부 시퀀스
// ─────────────────────────────────────────────────────────────────────────────

void UIntroSplashComponent::ShowIntroWidget()
{
	APlayerController* PC = GetOwnerController();
	if (!PC || !IntroWidgetClass)
	{
		return;
	}

	IntroWidgetInstance = CreateWidget<UIntroSplashWidget>(PC, IntroWidgetClass);
	if (!IntroWidgetInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("UIntroSplashComponent: 위젯 생성 실패."));
		return;
	}

	// 모든 게임플레이 UI 위에 표시
	IntroWidgetInstance->AddToViewport(9999);

	// FadeIn 애니메이션 시작
	IntroWidgetInstance->PlayFadeIn();
	bIntroActive = true;

	// 인트로 사운드 1회 재생 (UI 사운드로 처리 — bIsUISound=true)
	if (IntroSound)
	{
		UGameplayStatics::PlaySound2D(GetWorld(), IntroSound, 1.0f, 1.0f, 0.0f, nullptr, nullptr, true);
	}

	// FadeIn + Hold 시간이 지난 뒤 FadeOut 시작
	const float FadeOutTriggerDelay = FadeInDurationSeconds + HoldDurationSeconds;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			FadeOutTriggerHandle,
			this,
			&UIntroSplashComponent::BeginFadeOut,
			FadeOutTriggerDelay,
			false
		);
	}
}

void UIntroSplashComponent::BeginFadeOut()
{
	if (!IntroWidgetInstance)
	{
		return;
	}

	IntroWidgetInstance->PlayFadeOut();

	// FadeOut 애니메이션이 끝날 때쯤 위젯 제거
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			FinishHandle,
			this,
			&UIntroSplashComponent::FinishIntro,
			FadeOutDurationSeconds,
			false
		);
	}
}

void UIntroSplashComponent::FinishIntro()
{
	if (IntroWidgetInstance)
	{
		IntroWidgetInstance->RemoveFromParent();
		IntroWidgetInstance = nullptr;
	}

	bIntroActive = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// 공개 API
// ─────────────────────────────────────────────────────────────────────────────

void UIntroSplashComponent::SkipIntro()
{
	if (!bIntroActive)
	{
		return;
	}

	ClearAllTimers();
	FinishIntro();
}

// ─────────────────────────────────────────────────────────────────────────────
// 유틸리티
// ─────────────────────────────────────────────────────────────────────────────

APlayerController* UIntroSplashComponent::GetOwnerController() const
{
	return Cast<APlayerController>(GetOwner());
}

void UIntroSplashComponent::ClearAllTimers()
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(InitDelayHandle);
		TimerManager.ClearTimer(FadeOutTriggerHandle);
		TimerManager.ClearTimer(FinishHandle);
	}
}
