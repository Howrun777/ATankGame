#include "Shared/UI/ReturnToSpawnWidget.h"
#include "Components/ProgressBar.h" // 必须包含这个头文件

void UReturnToSpawnWidget::UpdateProgress(float InProgress)
{
	if (ReturnProgressBar)
	{
		// 直接在 C++ 中设置进度条的百分比 (0.0 到 1.0)
		ReturnProgressBar->SetPercent(InProgress);
	}
}