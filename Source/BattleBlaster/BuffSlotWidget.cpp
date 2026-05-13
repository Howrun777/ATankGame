#include "BuffSlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UBuffSlotWidget::UpdateSlot(UTexture2D* InIcon, float RemainingTime)
{
	// 1. 刷新图标（安全检查防崩溃）
	if (IconImage && InIcon)
	{
		IconImage->SetBrushFromTexture(InIcon);
	}

	// 2. 刷新文字和变红逻辑
	if (TimeText)
	{
		// 向上取整：比如还剩 5.1 秒，对玩家来说算是 6 秒
		int32 SecondsDisplay = FMath::CeilToInt(RemainingTime);
		TimeText->SetText(FText::AsNumber(SecondsDisplay));

		// 【你的需求】：<= 5 秒时文字变红
		if (RemainingTime <= 5.0f)
		{
			TimeText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
		}
		else
		{
			// 正常情况下是白色（你也可以改成你想要的颜色）
			TimeText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		}
	}
}
