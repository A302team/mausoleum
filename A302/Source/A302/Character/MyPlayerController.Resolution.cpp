#include "Character/MyPlayerController.h"

#include "Components/CheckBox.h"
#include "Components/ComboBoxString.h"
#include "Components/Slider.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

namespace
{
	constexpr float DefaultMouseSensitivityMultiplier = 1.0f;
	constexpr float MinMouseSensitivityMultiplier = 0.1f;
	constexpr float MaxMouseSensitivityMultiplier = 3.0f;
	const TCHAR* MouseSensitivityConfigSection = TEXT("/Script/A302.LocalSettings");
	const TCHAR* MouseSensitivityConfigKey = TEXT("MouseSensitivityMultiplier");
	const TCHAR* MasterVolumeConfigKey = TEXT("MasterVolume");
	const TCHAR* BGMVolumeConfigKey = TEXT("BGMVolume");
	const TCHAR* SFXVolumeConfigKey = TEXT("SFXVolume");
	const TCHAR* InterfaceVolumeConfigKey = TEXT("InterfaceVolume");
}

namespace
{
	void EnsureComboHasOption(UComboBoxString* ComboBox, const FString& Option)
	{
		if (!ComboBox || Option.IsEmpty())
		{
			return;
		}

		const int32 OptionCount = ComboBox->GetOptionCount();
		for (int32 OptionIndex = 0; OptionIndex < OptionCount; ++OptionIndex)
		{
			if (ComboBox->GetOptionAtIndex(OptionIndex).Equals(Option, ESearchCase::IgnoreCase))
			{
				return;
			}
		}

		ComboBox->AddOption(Option);
	}
}

void AMyPlayerController::EnsureVideoSettingOptions()
{
	if (ResolutionComboBox && ResolutionComboBox->GetOptionCount() == 0)
	{
		EnsureComboHasOption(ResolutionComboBox, TEXT("1280x720"));
		EnsureComboHasOption(ResolutionComboBox, TEXT("1600x900"));
		EnsureComboHasOption(ResolutionComboBox, TEXT("1920x1080"));
		EnsureComboHasOption(ResolutionComboBox, TEXT("2560x1440"));
	}

	if (FullscreenModeComboBox && FullscreenModeComboBox->GetOptionCount() == 0)
	{
		EnsureComboHasOption(FullscreenModeComboBox, TEXT("전체화면"));
		EnsureComboHasOption(FullscreenModeComboBox, TEXT("전체화면 창"));
		EnsureComboHasOption(FullscreenModeComboBox, TEXT("창모드"));
	}

	if (FrameLimitComboBox && FrameLimitComboBox->GetOptionCount() == 0)
	{
		EnsureComboHasOption(FrameLimitComboBox, TEXT("30"));
		EnsureComboHasOption(FrameLimitComboBox, TEXT("60"));
		EnsureComboHasOption(FrameLimitComboBox, TEXT("120"));
		EnsureComboHasOption(FrameLimitComboBox, TEXT("144"));
		EnsureComboHasOption(FrameLimitComboBox, TEXT("240"));
		EnsureComboHasOption(FrameLimitComboBox, TEXT("무제한"));
	}
}

void AMyPlayerController::SyncVideoSettingsToCurrent()
{
	SyncResolutionComboToCurrent();
	SyncFullscreenModeComboToCurrent();
	SyncFrameLimitComboToCurrent();
	SyncVSyncCheckBoxToCurrent();
}

void AMyPlayerController::LoadMouseSensitivitySetting()
{
	float LoadedValue = DefaultMouseSensitivityMultiplier;
	if (GConfig)
	{
		GConfig->GetFloat(MouseSensitivityConfigSection, MouseSensitivityConfigKey, LoadedValue, GGameUserSettingsIni);
	}

	MouseSensitivityMultiplier = FMath::Clamp(LoadedValue, MinMouseSensitivityMultiplier, MaxMouseSensitivityMultiplier);
}

void AMyPlayerController::SaveMouseSensitivitySetting(float NewValue)
{
	MouseSensitivityMultiplier = FMath::Clamp(NewValue, MinMouseSensitivityMultiplier, MaxMouseSensitivityMultiplier);

	if (GConfig)
	{
		GConfig->SetFloat(MouseSensitivityConfigSection, MouseSensitivityConfigKey, MouseSensitivityMultiplier, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

void AMyPlayerController::SyncMouseSensitivitySliderToCurrent()
{
	if (MouseSensitivitySlider)
	{
		MouseSensitivitySlider->SetValue(MouseSensitivityMultiplier);
	}
}

void AMyPlayerController::OnMouseSensitivitySliderChanged(float NewValue)
{
	MouseSensitivityMultiplier = FMath::Clamp(NewValue, MinMouseSensitivityMultiplier, MaxMouseSensitivityMultiplier);
}

void AMyPlayerController::LoadAudioSettings()
{
	MasterVolumeValue = 1.0f;
	BGMVolumeValue = 1.0f;
	SFXVolumeValue = 1.0f;
	InterfaceVolumeValue = 1.0f;

	if (GConfig)
	{
		GConfig->GetFloat(MouseSensitivityConfigSection, MasterVolumeConfigKey, MasterVolumeValue, GGameUserSettingsIni);
		GConfig->GetFloat(MouseSensitivityConfigSection, BGMVolumeConfigKey, BGMVolumeValue, GGameUserSettingsIni);
		GConfig->GetFloat(MouseSensitivityConfigSection, SFXVolumeConfigKey, SFXVolumeValue, GGameUserSettingsIni);
		GConfig->GetFloat(MouseSensitivityConfigSection, InterfaceVolumeConfigKey, InterfaceVolumeValue, GGameUserSettingsIni);
	}

	MasterVolumeValue = FMath::Clamp(MasterVolumeValue, 0.0f, 1.0f);
	BGMVolumeValue = FMath::Clamp(BGMVolumeValue, 0.0f, 1.0f);
	SFXVolumeValue = FMath::Clamp(SFXVolumeValue, 0.0f, 1.0f);
	InterfaceVolumeValue = FMath::Clamp(InterfaceVolumeValue, 0.0f, 1.0f);
}

void AMyPlayerController::SaveAudioSettings()
{
	MasterVolumeValue = FMath::Clamp(MasterVolumeValue, 0.0f, 1.0f);
	BGMVolumeValue = FMath::Clamp(BGMVolumeValue, 0.0f, 1.0f);
	SFXVolumeValue = FMath::Clamp(SFXVolumeValue, 0.0f, 1.0f);
	InterfaceVolumeValue = FMath::Clamp(InterfaceVolumeValue, 0.0f, 1.0f);

	if (GConfig)
	{
		GConfig->SetFloat(MouseSensitivityConfigSection, MasterVolumeConfigKey, MasterVolumeValue, GGameUserSettingsIni);
		GConfig->SetFloat(MouseSensitivityConfigSection, BGMVolumeConfigKey, BGMVolumeValue, GGameUserSettingsIni);
		GConfig->SetFloat(MouseSensitivityConfigSection, SFXVolumeConfigKey, SFXVolumeValue, GGameUserSettingsIni);
		GConfig->SetFloat(MouseSensitivityConfigSection, InterfaceVolumeConfigKey, InterfaceVolumeValue, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
}

void AMyPlayerController::SyncAudioSlidersToCurrent()
{
	if (MasterVolumeSlider)
	{
		MasterVolumeSlider->SetValue(MasterVolumeValue);
	}

	if (BGMVolumeSlider)
	{
		BGMVolumeSlider->SetValue(BGMVolumeValue);
	}

	if (SFXVolumeSlider)
	{
		SFXVolumeSlider->SetValue(SFXVolumeValue);
	}

	if (InterfaceVolumeSlider)
	{
		InterfaceVolumeSlider->SetValue(InterfaceVolumeValue);
	}
}

void AMyPlayerController::ApplyAudioSettings()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!RuntimeAudioSettingsMix)
	{
		RuntimeAudioSettingsMix = NewObject<USoundMix>(this, TEXT("RuntimeAudioSettingsMix"));
	}

	if (!RuntimeAudioSettingsMix)
	{
		return;
	}

	auto ApplyClassVolume = [&](USoundClass* SoundClass, float Volume)
	{
		if (!SoundClass)
		{
			return;
		}

		UGameplayStatics::SetSoundMixClassOverride(
			World,
			RuntimeAudioSettingsMix,
			SoundClass,
			FMath::Clamp(Volume, 0.0f, 1.0f),
			1.0f,
			0.05f,
			true);
	};

	ApplyClassVolume(MasterSoundClass, MasterVolumeValue);
	ApplyClassVolume(BGMSoundClass, BGMVolumeValue);
	ApplyClassVolume(SFXSoundClass, SFXVolumeValue);
	ApplyClassVolume(InterfaceSoundClass, InterfaceVolumeValue);

	UGameplayStatics::PushSoundMixModifier(World, RuntimeAudioSettingsMix);
}

void AMyPlayerController::SyncResolutionComboToCurrent()
{
	if (!ResolutionComboBox)
	{
		return;
	}

	FIntPoint CurrentResolution = FIntPoint::ZeroValue;

	if (UGameUserSettings* GameUserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		CurrentResolution = GameUserSettings->GetScreenResolution();
	}

	if (CurrentResolution.X <= 0 || CurrentResolution.Y <= 0)
	{
		int32 ViewportX = 0;
		int32 ViewportY = 0;
		GetViewportSize(ViewportX, ViewportY);
		CurrentResolution = FIntPoint(ViewportX, ViewportY);
	}

	if (CurrentResolution.X <= 0 || CurrentResolution.Y <= 0)
	{
		return;
	}

	const int32 OptionCount = ResolutionComboBox->GetOptionCount();
	for (int32 OptionIndex = 0; OptionIndex < OptionCount; ++OptionIndex)
	{
		const FString OptionText = ResolutionComboBox->GetOptionAtIndex(OptionIndex);
		FIntPoint ParsedResolution = FIntPoint::ZeroValue;
		if (TryParseResolutionString(OptionText, ParsedResolution) && ParsedResolution == CurrentResolution)
		{
			ResolutionComboBox->SetSelectedOption(OptionText);
			return;
		}
	}
}

void AMyPlayerController::SyncFullscreenModeComboToCurrent()
{
	if (!FullscreenModeComboBox)
	{
		return;
	}

	if (UGameUserSettings* GameUserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		switch (GameUserSettings->GetFullscreenMode())
		{
		case EWindowMode::Fullscreen:
			FullscreenModeComboBox->SetSelectedOption(TEXT("전체화면"));
			return;
		case EWindowMode::WindowedFullscreen:
			FullscreenModeComboBox->SetSelectedOption(TEXT("전체화면 창"));
			return;
		case EWindowMode::Windowed:
		default:
			FullscreenModeComboBox->SetSelectedOption(TEXT("창모드"));
			return;
		}
	}
}

void AMyPlayerController::SyncFrameLimitComboToCurrent()
{
	if (!FrameLimitComboBox)
	{
		return;
	}

	if (UGameUserSettings* GameUserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		const float FrameLimit = GameUserSettings->GetFrameRateLimit();
		if (FrameLimit <= 0.0f)
		{
			FrameLimitComboBox->SetSelectedOption(TEXT("무제한"));
			return;
		}

		FrameLimitComboBox->SetSelectedOption(FString::FromInt(FMath::RoundToInt(FrameLimit)));
	}
}

void AMyPlayerController::SyncVSyncCheckBoxToCurrent()
{
	if (!VSyncCheckBox)
	{
		return;
	}

	if (UGameUserSettings* GameUserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		VSyncCheckBox->SetIsChecked(GameUserSettings->IsVSyncEnabled());
	}
}

bool AMyPlayerController::TryParseResolutionString(const FString& InOption, FIntPoint& OutResolution) const
{
	FString Option = InOption;
	Option.TrimStartAndEndInline();

	FString XString;
	FString YString;
	if (!Option.Split(TEXT("x"), &XString, &YString, ESearchCase::IgnoreCase, ESearchDir::FromStart))
	{
		return false;
	}

	const int32 X = FCString::Atoi(*XString);
	const int32 Y = FCString::Atoi(*YString);
	if (X <= 0 || Y <= 0)
	{
		return false;
	}

	OutResolution = FIntPoint(X, Y);
	return true;
}

bool AMyPlayerController::TryParseFullscreenModeString(const FString& InOption, EWindowMode::Type& OutWindowMode) const
{
	FString Option = InOption;
	Option.TrimStartAndEndInline();

	if (Option.IsEmpty())
	{
		return false;
	}

	if (Option.Contains(TEXT("창모드")) || Option.Equals(TEXT("Windowed"), ESearchCase::IgnoreCase))
	{
		OutWindowMode = EWindowMode::Windowed;
		return true;
	}

	if (Option.Contains(TEXT("전체화면 창")) || Option.Contains(TEXT("테두리")) || Option.Equals(TEXT("Borderless"), ESearchCase::IgnoreCase) || Option.Equals(TEXT("WindowedFullscreen"), ESearchCase::IgnoreCase))
	{
		OutWindowMode = EWindowMode::WindowedFullscreen;
		return true;
	}

	if (Option.Contains(TEXT("전체화면")) || Option.Equals(TEXT("Fullscreen"), ESearchCase::IgnoreCase))
	{
		OutWindowMode = EWindowMode::Fullscreen;
		return true;
	}

	return false;
}

bool AMyPlayerController::TryParseFrameLimitString(const FString& InOption, float& OutFrameLimit) const
{
	FString Option = InOption;
	Option.TrimStartAndEndInline();

	if (Option.IsEmpty())
	{
		return false;
	}

	if (Option.Contains(TEXT("무제한")) || Option.Equals(TEXT("Unlimited"), ESearchCase::IgnoreCase) || Option.Equals(TEXT("Off"), ESearchCase::IgnoreCase))
	{
		OutFrameLimit = 0.0f;
		return true;
	}

	Option.ReplaceInline(TEXT("FPS"), TEXT(""), ESearchCase::IgnoreCase);
	Option.ReplaceInline(TEXT("fps"), TEXT(""), ESearchCase::IgnoreCase);
	Option.TrimStartAndEndInline();

	const float ParsedValue = FCString::Atof(*Option);
	if (ParsedValue <= 0.0f)
	{
		return false;
	}

	OutFrameLimit = ParsedValue;
	return true;
}

void AMyPlayerController::OnResolutionApplyClicked()
{
	if (!ResolutionComboBox && !FullscreenModeComboBox && !FrameLimitComboBox && !VSyncCheckBox)
	{
		return;
	}
	FIntPoint TargetResolution = FIntPoint::ZeroValue;
	EWindowMode::Type TargetWindowMode = EWindowMode::Windowed;
	float TargetFrameLimit = 0.0f;
	bool bHasResolutionSelection = false;
	bool bHasWindowModeSelection = false;
	bool bHasFrameLimitSelection = false;

	if (ResolutionComboBox)
	{
		const FString SelectedResolution = ResolutionComboBox->GetSelectedOption();
		if (!SelectedResolution.IsEmpty())
		{
			bHasResolutionSelection = TryParseResolutionString(SelectedResolution, TargetResolution);
			if (!bHasResolutionSelection)
			{
				UE_LOG(LogTemp, Warning, TEXT("[PC] Invalid resolution option: %s"), *SelectedResolution);
			}
		}
	}

	if (FullscreenModeComboBox)
	{
		const FString SelectedWindowMode = FullscreenModeComboBox->GetSelectedOption();
		if (!SelectedWindowMode.IsEmpty())
		{
			bHasWindowModeSelection = TryParseFullscreenModeString(SelectedWindowMode, TargetWindowMode);
			if (!bHasWindowModeSelection)
			{
				UE_LOG(LogTemp, Warning, TEXT("[PC] Invalid fullscreen mode option: %s"), *SelectedWindowMode);
			}
		}
	}

	if (FrameLimitComboBox)
	{
		const FString SelectedFrameLimit = FrameLimitComboBox->GetSelectedOption();
		if (!SelectedFrameLimit.IsEmpty())
		{
			bHasFrameLimitSelection = TryParseFrameLimitString(SelectedFrameLimit, TargetFrameLimit);
			if (!bHasFrameLimitSelection)
			{
				UE_LOG(LogTemp, Warning, TEXT("[PC] Invalid frame limit option: %s"), *SelectedFrameLimit);
			}
		}
	}

	if (UGameUserSettings* GameUserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		if (bHasWindowModeSelection)
		{
			GameUserSettings->SetFullscreenMode(TargetWindowMode);
		}

		if (bHasResolutionSelection)
		{
			GameUserSettings->SetScreenResolution(TargetResolution);
		}

		if (bHasFrameLimitSelection)
		{
			GameUserSettings->SetFrameRateLimit(TargetFrameLimit);
		}

		if (VSyncCheckBox)
		{
			GameUserSettings->SetVSyncEnabled(VSyncCheckBox->IsChecked());
		}

		if (MouseSensitivitySlider)
		{
			SaveMouseSensitivitySetting(MouseSensitivitySlider->GetValue());
		}

		if (MasterVolumeSlider)
		{
			MasterVolumeValue = MasterVolumeSlider->GetValue();
		}

		if (BGMVolumeSlider)
		{
			BGMVolumeValue = BGMVolumeSlider->GetValue();
		}

		if (SFXVolumeSlider)
		{
			SFXVolumeValue = SFXVolumeSlider->GetValue();
		}

		if (InterfaceVolumeSlider)
		{
			InterfaceVolumeValue = InterfaceVolumeSlider->GetValue();
		}

		SaveAudioSettings();
		ApplyAudioSettings();

		GameUserSettings->ApplyResolutionSettings(false);
		GameUserSettings->ApplySettings(false);
		GameUserSettings->SaveSettings();

		if (bHasResolutionSelection)
		{
			const TCHAR WindowModeSuffix = (bHasWindowModeSelection && TargetWindowMode == EWindowMode::Windowed) ? TEXT('w') : TEXT('f');
			const FString SetResCommand = FString::Printf(TEXT("r.SetRes %dx%d%c"), TargetResolution.X, TargetResolution.Y, WindowModeSuffix);
			ConsoleCommand(SetResCommand, true);
		}

		UE_LOG(LogTemp, Log, TEXT("[PC] Applied video settings"));
		SyncVideoSettingsToCurrent();
		SyncMouseSensitivitySliderToCurrent();
		SyncAudioSlidersToCurrent();
	}
}
