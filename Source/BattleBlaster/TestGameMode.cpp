#include "TestGameMode.h"






#include "Kismet/GameplayStatics.h" // 对应蓝图的 GetPlayerController 等常用函数库

void ATestGameMode::BeginPlay()
{
	Super::BeginPlay();

// 1. 检查有没有设置 UI 类 (防止崩)
if (TestUIWidgetClass)
{
	// 2. Create Widget
	TestUIWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), TestUIWidgetClass);

	if (TestUIWidgetInstance)
	{
		// 3. Add to Viewport
		TestUIWidgetInstance->AddToViewport();

		// 4. 显示鼠标 (对应 Get Player Controller -> Set Show Mouse Cursor)
		APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
		if (PC)
		{
			PC->bShowMouseCursor = true;
			PC->SetInputMode(FInputModeUIOnly()); // 设置输入模式
		}
	}
}
}