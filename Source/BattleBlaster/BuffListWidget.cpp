#include "BuffListWidget.h"
#include "Components/VerticalBox.h"
#include "Kismet/GameplayStatics.h"
#include "TankPlayerController.h" 
#include "Tank.h" 
#include "TankBuffComponent.h" // 确保引入了你的 Buff 组件头文件
#include "BuffSlotWidget.h"    // 必须引入刚写的子类头文件

void UBuffListWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateBuffList();
}

void UBuffListWidget::UpdateBuffList()
{
	// 安全检查
	if (!BuffContainer || !BuffSlotClass) return;

	// 从自己所属的玩家控制器拿Pawn，而不是硬编码0号玩家
	ATank* PlayerTank = Cast<ATank>(OwnerPlayerController->GetPawn());
	if (!PlayerTank || !PlayerTank->GetBuffComponent()) return;

	// 获取正在持续的 Buff 数组 (不含一次性的)
	TArray<FActiveBuffUIInfo> ActiveBuffs = PlayerTank->GetBuffComponent()->GetActiveBuffsForUI();

	// 2. 核心：动态控制槽位的数量和显示
	int32 NeededSlots = ActiveBuffs.Num();
	int32 CurrentSlots = BuffContainer->GetChildrenCount();

	// 如果目前的槽位不够用，就创建几个新的塞进垂直框里
	while (CurrentSlots < NeededSlots)
	{
		UBuffSlotWidget* NewSlot = CreateWidget<UBuffSlotWidget>(this, BuffSlotClass);
		if (NewSlot)
		{
			BuffContainer->AddChildToVerticalBox(NewSlot);
		}
		CurrentSlots++;
	}


	// 遍历所有存在的槽位
	for (int32 i = 0; i < CurrentSlots; i++)
	{
		UBuffSlotWidget* BuffSlot = Cast<UBuffSlotWidget>(BuffContainer->GetChildAt(i));
		if (BuffSlot)
		{
			// 如果这个槽位是需要的
			if (i < NeededSlots)
			{
				BuffSlot->SetVisibility(ESlateVisibility::Visible); // 确保显示出来
				// 把数据注入给这个子槽位，让它自己去刷新图标和文字
				BuffSlot->UpdateSlot(ActiveBuffs[i].Icon, ActiveBuffs[i].RemainingTime);
			}
			else
			{
				// 多出来的槽位，不要销毁，直接折叠起来（不占用屏幕空间和排版）
				BuffSlot->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

void UBuffListWidget::InitBuffUI(ATankPlayerController* InOwnerController)
{
	if (IsValid(InOwnerController))
	{
		OwnerPlayerController = InOwnerController;
	}
}