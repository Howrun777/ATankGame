#include "Shared/UI/BuffSlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UBuffSlotWidget::UpdateSlot(UTexture2D* InIcon, float RemainingTime)
{
	// 1. 刷新图标（安全检查防崩溃）
	if (IconImage && InIcon && CachedIcon != InIcon)
	{
		IconImage->SetBrushFromTexture(InIcon);
		CachedIcon = InIcon;
	}

	// 2. 刷新文字和变红逻辑
	if (TimeText)
	{
		// 向上取整：比如还剩 5.1 秒，对玩家来说算是 6 秒
		const int32 SecondsDisplay = FMath::CeilToInt(RemainingTime);
		const bool bFirstTimeUpdate = CachedSecondsDisplay == INDEX_NONE;
		if (bFirstTimeUpdate || CachedSecondsDisplay != SecondsDisplay)
		{
			TimeText->SetText(FText::AsNumber(SecondsDisplay));
			CachedSecondsDisplay = SecondsDisplay;
		}

		// 【你的需求】：<= 5 秒时文字变红
		const bool bShouldUseWarningColor = RemainingTime <= 5.0f;
		if (bFirstTimeUpdate || bCachedWarningColor != bShouldUseWarningColor)
		{
			TimeText->SetColorAndOpacity(FSlateColor(bShouldUseWarningColor ? FLinearColor::Red : FLinearColor::White));
			bCachedWarningColor = bShouldUseWarningColor;
		}
	}
}
