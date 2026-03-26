#include "UI/EscapeWaitingWidget.h"

#include "Components/TextBlock.h"
#include "Components/Widget.h"

void UEscapeWaitingWidget::UpdateSpectatorTargetName(const FString& TargetName)
{
	if (Text_SpectatorTargetName)
	{
		Text_SpectatorTargetName->SetText(FText::FromString(TargetName));
	}

	// 관전 대상이 없으면(자기 자신 or 모두 탈출) 관전 안내 영역 숨김
	if (Panel_SpectatorInfo)
	{
		Panel_SpectatorInfo->SetVisibility(
			TargetName.IsEmpty()
				? ESlateVisibility::Collapsed
				: ESlateVisibility::Visible
		);
	}
}
