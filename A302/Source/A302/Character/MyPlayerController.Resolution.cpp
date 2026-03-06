#include "Character/MyPlayerController.h"

#include "Components/ComboBoxString.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"

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

void AMyPlayerController::OnResolutionApplyClicked()
{
	if (!ResolutionComboBox)
	{
		return;
	}

	const FString SelectedOption = ResolutionComboBox->GetSelectedOption();
	if (SelectedOption.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] Resolution option is empty"));
		return;
	}

	FIntPoint TargetResolution = FIntPoint::ZeroValue;
	if (!TryParseResolutionString(SelectedOption, TargetResolution))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] Invalid resolution option: %s"), *SelectedOption);
		return;
	}

	if (UGameUserSettings* GameUserSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		// In editor/standalone, borderless fullscreen can block downsizing.
		// Force windowed mode so both upsize and downsize work consistently.
		GameUserSettings->SetFullscreenMode(EWindowMode::Windowed);
		GameUserSettings->SetScreenResolution(TargetResolution);
		GameUserSettings->ApplyResolutionSettings(false);
		GameUserSettings->ApplySettings(false);
		GameUserSettings->SaveSettings();

		const FString SetResCommand = FString::Printf(TEXT("r.SetRes %dx%dw"), TargetResolution.X, TargetResolution.Y);
		ConsoleCommand(SetResCommand, true);

		UE_LOG(LogTemp, Log, TEXT("[PC] Applied resolution: %s"), *SelectedOption);
		SyncResolutionComboToCurrent();
	}
}
