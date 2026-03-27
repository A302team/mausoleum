#include "UI/IntroSplashLocalPlayerSubsystem.h"

#include "UI/IntroSplashWidget.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

void UIntroSplashLocalPlayerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (IntroWidgetClass.IsNull())
	{
		IntroWidgetClass = TSoftClassPtr<UIntroSplashWidget>(FSoftObjectPath(TEXT("/Game/WorkSpace/UI/WBP_IntroSplash.WBP_IntroSplash_C")));
	}

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UWorld* World = LocalPlayer->GetWorld())
		{
			TryScheduleIntro(LocalPlayer->GetPlayerController(World));
		}
	}
}

void UIntroSplashLocalPlayerSubsystem::Deinitialize()
{
	ClearTimers();
	FinishIntro();
	ActiveWorld.Reset();
	bIntroQueued = false;

	Super::Deinitialize();
}

void UIntroSplashLocalPlayerSubsystem::PlayerControllerChanged(APlayerController* NewPlayerController)
{
	Super::PlayerControllerChanged(NewPlayerController);
	TryScheduleIntro(NewPlayerController);
}

void UIntroSplashLocalPlayerSubsystem::TryScheduleIntro(APlayerController* PlayerController)
{
	if (bIntroShown || bIntroQueued)
	{
		return;
	}

	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	UWorld* World = PlayerController->GetWorld();
	if (!World)
	{
		return;
	}

	if (World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	if (World->WorldType != EWorldType::Game && World->WorldType != EWorldType::PIE)
	{
		return;
	}

	ActiveWorld = World;
	World->GetTimerManager().SetTimer(
		InitDelayHandle,
		this,
		&UIntroSplashLocalPlayerSubsystem::ShowIntroWidget,
		InitDelaySeconds,
		false
	);
	bIntroQueued = true;
}

void UIntroSplashLocalPlayerSubsystem::ShowIntroWidget()
{
	if (bIntroShown)
	{
		return;
	}

	UWorld* World = ActiveWorld.Get();
	ULocalPlayer* LocalPlayer = GetLocalPlayer();
	if (!World || !LocalPlayer)
	{
		return;
	}

	APlayerController* PC = LocalPlayer->GetPlayerController(World);
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	UClass* WidgetClass = IntroWidgetClass.LoadSynchronous();
	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[IntroSplashSubsystem] Intro widget class is not available."));
		return;
	}

	IntroWidgetInstance = CreateWidget<UIntroSplashWidget>(PC, WidgetClass);
	if (!IntroWidgetInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[IntroSplashSubsystem] Failed to create intro widget."));
		return;
	}

	IntroWidgetInstance->AddToViewport(9999);
	IntroWidgetInstance->PlayFadeIn();

	if (USoundBase* IntroSoundAsset = IntroSound.LoadSynchronous())
	{
		UGameplayStatics::PlaySound2D(World, IntroSoundAsset, 1.0f, 1.0f, 0.0f, nullptr, nullptr, true);
	}

	const float FadeOutDelay = FadeInDurationSeconds + HoldDurationSeconds;
	World->GetTimerManager().SetTimer(
		FadeOutTriggerHandle,
		this,
		&UIntroSplashLocalPlayerSubsystem::BeginFadeOut,
		FadeOutDelay,
		false
	);

	bIntroShown = true;
	bIntroQueued = false;
}

void UIntroSplashLocalPlayerSubsystem::BeginFadeOut()
{
	UWorld* World = ActiveWorld.Get();
	if (!World || !IntroWidgetInstance)
	{
		return;
	}

	IntroWidgetInstance->PlayFadeOut();
	World->GetTimerManager().SetTimer(
		FinishHandle,
		this,
		&UIntroSplashLocalPlayerSubsystem::FinishIntro,
		FadeOutDurationSeconds,
		false
	);
}

void UIntroSplashLocalPlayerSubsystem::FinishIntro()
{
	if (IntroWidgetInstance)
	{
		IntroWidgetInstance->RemoveFromParent();
		IntroWidgetInstance = nullptr;
	}
}

void UIntroSplashLocalPlayerSubsystem::ClearTimers()
{
	if (UWorld* World = ActiveWorld.Get())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(InitDelayHandle);
		TimerManager.ClearTimer(FadeOutTriggerHandle);
		TimerManager.ClearTimer(FinishHandle);
	}
}
