// Fill out your copyright notice in the Description page of Project Settings.


#include "Modes/MainMenu/UI/GameSettingsMenuWidget.h"

#include "Components/WidgetSwitcher.h"
#include "Components/Button.h"

bool UGameSettingsMenuWidget::Initialize()
{
	if (!Super::Initialize()) return false;

	// 绑定点击事件
	if (Btn_TabGamepad) Btn_TabGamepad->OnClicked.AddDynamic(this, &UGameSettingsMenuWidget::OnGamepadTabClicked);
	if (Btn_TabKBM)     Btn_TabKBM->OnClicked.AddDynamic(this, &UGameSettingsMenuWidget::OnKBMTabClicked);
	if (Btn_TabCustom)  Btn_TabCustom->OnClicked.AddDynamic(this, &UGameSettingsMenuWidget::OnCustomTabClicked);
	if (Btn_Back)       Btn_Back->OnClicked.AddDynamic(this, &UGameSettingsMenuWidget::OnBackClicked);

	// 默认显示第 0 页（手柄页面）
	if (ContentSwitcher)
	{
		ContentSwitcher->SetActiveWidgetIndex(0);
		UpdateTabButtonStyles(0);
	}

	return true;
}

void UGameSettingsMenuWidget::OnGamepadTabClicked()
{
	// 切换到第 0 个子控件
	if (ContentSwitcher) ContentSwitcher->SetActiveWidgetIndex(0);
	UpdateTabButtonStyles(0);
}

void UGameSettingsMenuWidget::OnKBMTabClicked()
{
	// 切换到第 1 个子控件
	if (ContentSwitcher) ContentSwitcher->SetActiveWidgetIndex(1);
	UpdateTabButtonStyles(1);
}

void UGameSettingsMenuWidget::OnCustomTabClicked()
{
	// 切换到第 2 个子控件
	if (ContentSwitcher) ContentSwitcher->SetActiveWidgetIndex(2);
	UpdateTabButtonStyles(2);
}

void UGameSettingsMenuWidget::OnBackClicked()
{
	// 如果有 ParentUI（从主菜单打开的情况），显示父界面
	if (ParentUI)
	{
		ParentUI->SetVisibility(ESlateVisibility::Visible);
		RemoveFromParent();
	}
	else
	{
		// 没有 ParentUI，默认移除自己
		RemoveFromParent();
	}
}

void UGameSettingsMenuWidget::UpdateTabButtonStyles(int32 ActiveIndex)
{
	// 这是一个细节优化：当选中某个 Tab 时，让这个按钮处于“不可点击”或“高亮”状态
	// 其他按钮恢复正常状态
	if (Btn_TabGamepad) Btn_TabGamepad->SetIsEnabled(ActiveIndex != 0);
	if (Btn_TabKBM)     Btn_TabKBM->SetIsEnabled(ActiveIndex != 1);
	if (Btn_TabCustom)  Btn_TabCustom->SetIsEnabled(ActiveIndex != 2);

	// 进阶做法：你可以在这里更换 ActiveIndex 对应按钮的底图材质，实现真正的“高亮选中”效果
}