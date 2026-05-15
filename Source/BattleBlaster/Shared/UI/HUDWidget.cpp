// Fill out your copyright notice in the Description page of Project Settings.


#include "Shared/UI/HUDWidget.h"

//进度条更新函数
void UHUDWidget::SetHealthBarPercent(float NewPercent)
{
	if (HealthBar && NewPercent >= 0.0f && NewPercent <= 1.0f)
	{
		HealthBar->SetPercent(NewPercent);
	}
}

void UHUDWidget::SetShieldBarPercent(float NewPercent)
{
	if (ShieldBar)
	{
		float Clamped = FMath::Clamp(NewPercent, 0.0f, 1.0f);
		ShieldBar->SetPercent(Clamped);
		ShieldBar->SetVisibility(Clamped > 0.0f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}
