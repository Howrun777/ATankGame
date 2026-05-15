// Fill out your copyright notice in the Description page of Project Settings.

#include "Modes/MOBA/UI/EliminatedScreenWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UEliminatedScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 绑定按钮点击事件
	if (Btn_SwitchSpectate)
	{
		Btn_SwitchSpectate->OnClicked.AddDynamic(this, &UEliminatedScreenWidget::OnSwitchSpectateClickedInternal);
	}

	// 默认隐藏
	Hide();
}

void UEliminatedScreenWidget::Show()
{
	if (BackgroundBorder)
	{
		BackgroundBorder->SetVisibility(ESlateVisibility::Visible);
	}
}

void UEliminatedScreenWidget::Hide()
{
	if (BackgroundBorder)
	{
		BackgroundBorder->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UEliminatedScreenWidget::OnSwitchSpectateClicked()
{
	// 供蓝图重写或调用
}

void UEliminatedScreenWidget::OnSwitchSpectateClickedInternal()
{
	OnSwitchSpectateClicked();
}
